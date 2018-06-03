#include <rendercat/shader_set.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <tinyheaders/cute_files.h>
#include <fmt/core.h>
#include <sstream>
#include <fstream>
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

	fmt::print(stderr, "[shader]  could not determine type of file: [{}]\n", filename);

	return GL_INVALID_ENUM;
}

struct Shader
{
	std::string     filepath;
	cf_time_t       last_mod = {};
	rc::shader_handle   handle;

	explicit Shader(std::string&& fpath) : filepath(std::move(fpath)) { }

	~Shader() = default;
	Shader(Shader&& o) noexcept = default;
	Shader& operator =(Shader&& o) noexcept = default;
	RC_DISABLE_COPY(Shader)

	bool should_reload()
	{
		cf_time_t ft;
		cf_get_file_time(filepath.data(), &ft);

		if(likely(0 == cf_compare_file_times(&last_mod, &ft))) {
			return false;
		}
		last_mod = ft;
		return true;
	}

	bool reload() try
	{
		if(!should_reload())
			return false;

		std::ifstream filestream;
		filestream.open(filepath);

		if(!filestream.is_open()) {
			fmt::print(stderr, "[shader]  reload failed: could not open file [{}]\n", filepath);
			std::fflush(stderr);
		}

		std::ostringstream oss{};
		oss << filestream.rdbuf();

		auto shader_type = shader_type_from_filename(filepath);
		const auto shader_source = oss.str();
		const auto shader_source_ptr = shader_source.data();

		rc::shader_handle new_handle(glCreateShader(shader_type));
		glShaderSource(*new_handle, 1, &shader_source_ptr, nullptr);
		glCompileShader(*new_handle);
		GLint success = 0;
		glGetShaderiv(*new_handle, GL_COMPILE_STATUS, &success);
		if(success && new_handle) {
			handle = std::move(new_handle);
			fmt::print("[shader]  reload success [{}]\n", filepath);
			std::fflush(stdout);
			return true;
		}
		return false;

	} catch(const std::exception& e) {
		fmt::print(stderr, "[shader]  exception, what(): {}\n", e.what());
		std::fflush(stderr);
		return false;
	}

	bool valid() const noexcept
	{
		return static_cast<bool>(handle);
	}
};

}

class ShaderSet::Program
{
	std::vector<Shader> m_shaders;
public:
	rc::program_handle handle{};

	Program() = default;
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
		std::destroy_at(m_programs[i]);
		m_programs[i] = nullptr;
	}
}

bool ShaderSet::check_updates()
{
	bool ret = false;
	for(unsigned i = 0; i < m_program_count; ++i) {
		ret |= (m_programs[i])->reload();
	}
	return ret;
}


gl::GLuint * ShaderSet::load_program(std::initializer_list<std::string_view> names)
{
	Program program;

	for(const auto& name : names) {
		std::string filepath(m_directory);
		filepath.append(name);

		if(!cf_file_exists(filepath.data())) {
			fmt::print(stderr, "[shader]  shader file does not exist: [{}]\n", filepath);
			return nullptr;
		}

		program.attach_shader(Shader(std::move(filepath)));
	}

	program.reload();

	if(program.valid()) {
		m_programs[m_program_count] = new Program{std::move(program)};
		return m_programs[m_program_count++]->handle.get();
	}

	return nullptr;

}



