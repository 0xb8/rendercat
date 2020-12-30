#include <rendercat/shader_set.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <fmt/format.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <memory>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;
using namespace rc;

namespace {

struct shader_ext_type
{
	const char* const ext;
	const GLenum type;
};

constexpr shader_ext_type types[] {
	{".vert", GL_VERTEX_SHADER},
	{".frag", GL_FRAGMENT_SHADER},
	{".geom", GL_GEOMETRY_SHADER},
	{".comp", GL_COMPUTE_SHADER},
	{".tese", GL_TESS_EVALUATION_SHADER},
	{".tesc", GL_TESS_CONTROL_SHADER}
};


std::string_view get_include(std::string_view s) {
	auto pos = s.find("#include");
	if (pos != s.npos) {
		pos = s.find_first_of("\"<", pos);
		if (pos != s.npos) {
			auto end = s.find_first_of("\">", pos+1);
			if (end != s.npos) {
				auto res = s.substr(pos + 1, end-pos-1);
				return res;
			}
		}
	}
	return std::string_view{};
}

std::string make_definition(std::string_view name, std::string_view val) {
	return fmt::format("#define {} {}\n", name, val);
}

std::string make_line(unsigned file, unsigned line) {
	return fmt::format("#line {} {}\n", line, file);
}

std::string load_and_process_shader(
	const std::filesystem::path& root_dir,
	const std::filesystem::path& filepath,
	int depth,
	const ShaderSet::macros_t& definitions = ShaderSet::macros_t{})
{

	std::ifstream filestream;
	filestream.open(filepath);

	if(!filestream.is_open()) {
		throw std::runtime_error(fmt::format("could not open {} file [{}]\n",
		                                     depth > 0 ? "include" : "",
		                                     filepath.string()));
	}

	std::string result;
	size_t current_line = 0;
	bool version_found = false;

	auto append_line = [&result](std::string_view line) {
		result.append(line);
		result.push_back('\n');
	};

	std::string line;
	while(std::getline(filestream, line)) {
		++current_line;

		if (!version_found) {
			if (line.find("#version") != line.npos) {
				append_line(line);
				version_found = true;

				result.append(make_definition("OPENGL", ""));
				for (const auto& macro: definitions) {
					result.append(macro.get_define_string());
				}
				result.append(make_line(0, current_line+1));
				continue;
			}
		}

		if (line.find("#include") != line.npos) {
			auto path = get_include(line);
			auto p = root_dir / path;
			auto source = load_and_process_shader(root_dir, p, depth+1);
			if (source.empty()) {
				throw std::runtime_error(fmt::format("could not open include file [{}]\n", p.string()));
			}
			result.append(make_line(depth+1, 1));
			result.append(source);
			result.push_back('\n');
			result.append(make_line(depth, current_line+1));
			continue;
		}

		append_line(line);
	}
	return result;
}


GLenum shader_type_from_filename(const std::string_view filename)
{
	auto ext_pos = filename.find_last_of('.');
	if(ext_pos != std::string_view::npos && ext_pos < filename.length()-4) {
		auto ext = filename.substr(ext_pos);

		for(unsigned i = 0; i < std::size(types); ++i) {
			if(ext == types[i].ext)
				return types[i].type;

		}
	}

	throw std::runtime_error("could not determine type of shader file");
}

struct Shader
{
	std::filesystem::path           filepath;
	std::filesystem::path           root_path;
	std::filesystem::file_time_type last_write_time;
	rc::shader_handle               handle;
	const ShaderSet::macros_t*      definitions = nullptr;

	explicit Shader(std::filesystem::path&& root_path, std::filesystem::path&& fpath) :
	        filepath(std::move(fpath)),
	        root_path(std::move(root_path))
	{ }

	~Shader() = default;
	Shader(Shader&& o) noexcept = default;
	Shader& operator =(Shader&& o) noexcept = default;
	RC_DISABLE_COPY(Shader)

	void set_definitions(const ShaderSet::macros_t& macros) {
		definitions = &macros;
	}

	bool should_reload()
	{
		auto file_time = std::filesystem::last_write_time(filepath);
		if(likely(file_time == last_write_time)) {
			return false;
		}
		last_write_time = file_time;
		return true;
	}

	bool reload() try
	{
		if(!should_reload())
			return false;

		auto shader_type = shader_type_from_filename(filepath.string());
		static const auto empty_defs = ShaderSet::macros_t{};
		const auto shader_source = load_and_process_shader(root_path / "include", filepath, 0, definitions ? *definitions : empty_defs);
		const auto shader_source_ptr = shader_source.data();

		rc::shader_handle new_handle(glCreateShader(shader_type));
		glShaderSource(*new_handle, 1, &shader_source_ptr, nullptr);
		glCompileShader(*new_handle);
		GLint success = 0;
		glGetShaderiv(*new_handle, GL_COMPILE_STATUS, &success);
		if(success && new_handle) {
			handle = std::move(new_handle);
			fmt::print("[shader]  reload success [{}]\n", filepath.string());
			std::fflush(stdout);
			return true;
		}
		return false;

	} catch(const std::exception& e) {
		fmt::print(stderr, "[shader]  reload failed  [{}]: {}\n", filepath.string(), e.what());
		std::fflush(stderr);
		return false;
	}

	bool valid() const noexcept
	{
		return static_cast<bool>(handle);
	}

	const std::filesystem::path& path() const {
		return filepath;
	}
};

} // anonymous namespace

class ShaderSet::Program
{
	std::vector<Shader> m_shaders;
	macros_t m_macros;
public:
	rc::program_handle handle{};

	explicit Program(macros_t&& macros) : m_macros(std::move(macros)){ }
	~Program() = default;
	Program(Program&&) noexcept = default;
	Program& operator=(Program&&) noexcept = default;
	RC_DISABLE_COPY(Program)

	static bool link(rc::program_handle& program)
	{
		glLinkProgram(*program);
		GLint success = 0;
		glGetProgramiv(*program, GL_LINK_STATUS, &success);
		if(!success) {
			fmt::print(stderr, "[shader]  failed to link program id [{}]\n", *program);
		}
		return success;
	}

	void attach_shader(Shader&& s)
	{
		s.set_definitions(m_macros);
		m_shaders.push_back(std::move(s));
	}

	bool reload()
	{
		unsigned valid_shaders = 0, reloaded_shaders = 0;
		for(auto& s : m_shaders) {
			if(s.reload()) {
				++reloaded_shaders;
			}
			if(s.valid()) {
				++valid_shaders;
			}
		}
		if(reloaded_shaders == 0 || valid_shaders != m_shaders.size())
			return false;

		rc::program_handle new_handle(glCreateProgram());

		for(auto& s : m_shaders) {
			glAttachShader(*new_handle, *s.handle);
		}

		if(Program::link(new_handle)) {
			handle = std::move(new_handle);
			return true;
		}

		std::vector<std::string> files;
		for (const auto& shader : m_shaders)
			files.push_back(shader.path().string());
		fmt::print(stderr, "[shader]  failed shaders:\n"
		                   "[shader]    {}\n", fmt::join(files, "\n[shader]    "));
		std::vector<std::string> defines;
		for(const auto& macro: m_macros) {
			auto ds = macro.get_define_string();
			defines.push_back(ds.substr(0, ds.size()-2));
		}
		fmt::print(stderr, "[shader]  failed macros:\n"
		                   "[shader]    {}\n", fmt::join(defines, "\n[shader]    "));

		// seems like there's no need to detach shaders before program deletion
		return false;
	}


	bool valid() const
	{
		return static_cast<bool>(handle);
	}


};


ShaderSet::ShaderSet(std::string_view directory) : m_directory(directory) { }

ShaderSet::~ShaderSet()
{
	for(unsigned i = 0; i < m_program_count; ++i) {
		if (m_programs[i]) {
			std::destroy_at(m_programs[i]);
			m_programs[i] = nullptr;
		}
	}
}

void ShaderSet::check_updates()
{
	if (m_dirty) {
		for(unsigned i = 0; i < m_program_count; ++i) {
			auto prog = m_programs[i];
			if (prog)
				prog->reload();
		}
		m_dirty = false;
	}

	if (m_program_count > 0) {
		auto prog = m_programs[m_next_reload_index];
		if (prog)
			prog->reload();

		m_next_reload_index = (m_next_reload_index + 1) % m_program_count;
	}
}

gl::GLuint * ShaderSet::load_program(std::vector<std::string>&& names, macros_t&& defines)
{
	auto program = std::make_unique<Program>(std::move(defines));

	for(const auto& name : names) {

		std::filesystem::path dirpath = m_directory;
		auto filepath = dirpath / name;

		if(!std::filesystem::exists(filepath)) {
			fmt::print(stderr, "[shader]  shader file does not exist: [{}]\n", filepath.string());
			return nullptr;
		}

		program->attach_shader(Shader(std::move(dirpath), std::move(filepath)));
	}

	program->reload();

	if(program->valid()) {
		m_programs[m_program_count] = program.release();
		m_dirty = true;
		return m_programs[m_program_count++]->handle.get();
	}

	return nullptr;
}


bool ShaderSet::deleteProgram(uint32_t** p)
{
	if (p) {
		for (unsigned i = 0; i < m_program_count; ++i) {
			if (m_programs[i] && (m_programs[i]->handle.get() == *p)) {
				delete m_programs[i];
				m_programs[i] = nullptr;
				*p = nullptr;
				return true;
			}
		}
	}
	return false;
}




ShaderMacro::ShaderMacro(std::string_view name, std::string_view value) : m_name(name), m_value(value) { }

ShaderMacro::ShaderMacro(std::string_view name, float value) : ShaderMacro(name, std::to_string(value)) { }

ShaderMacro::ShaderMacro(std::string_view name, int value) : ShaderMacro(name, std::to_string(value)) { }

ShaderMacro::ShaderMacro(std::string_view name, unsigned value) : ShaderMacro(name, std::to_string(value)) { }

ShaderMacro::ShaderMacro(std::string_view name, bool value) : ShaderMacro(name, std::to_string(value)) { }

ShaderMacro::ShaderMacro(std::string_view name, double value) : ShaderMacro(name, std::to_string(value)) { }

std::string ShaderMacro::get_define_string() const
{
	return make_definition(m_name, m_value);
}
