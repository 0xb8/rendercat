#include <shader_set.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>
#include <vector>

#define VERT_SHADER_EXT ".vert"
#define FRAG_SHADER_EXT ".frag"

namespace fs = boost::filesystem;
namespace {

struct Shader
{
	explicit Shader(const fs::path&& fpath) : filepath(fpath)
	{
		reload(true);
	}

	~Shader()
	{
		glDeleteShader(handle);
	}

	bool should_reload()
	{
		auto mod_date = fs::last_write_time(filepath);

		if(last_mod >= mod_date)
		{
			return false;
		}
		return true;
	}

	void reload(bool force) try
	{
		fs::ifstream sh_fstream;
		sh_fstream.open(filepath);

		auto filename_str = filepath.string();
		auto filename = std::string_view{filename_str};

		if(!sh_fstream.is_open()) {
			std::cerr << "[shader]   could not open " << filename << std::endl;
			return;
		}

		if(!force && !should_reload())
		{
			return;
		}

		last_mod = fs::last_write_time(filepath);

		auto ext_pos = filename.find_last_of('.');
		if(ext_pos == std::string_view::npos || ext_pos == filename.length()-1) {
			std::cerr << "[shader]   could not determine shader type of " << filename << std::endl;
			return;
		}
		auto ext = filename.substr(ext_pos);

		int shader_type = 0;
		if(ext == VERT_SHADER_EXT) {
			shader_type = GL_VERTEX_SHADER;
		}
		else if(ext == FRAG_SHADER_EXT) {
			shader_type = GL_FRAGMENT_SHADER;
		}
		else {
			std::cerr << "[shader]   unknown shader type: " << ext << " of " << filename <<std::endl;
			return;
		}

		std::ostringstream oss{};
		oss << sh_fstream.rdbuf();
		auto shader_source = oss.str();
		auto shader_source_data = shader_source.c_str();


		GLuint new_handle = glCreateShader(shader_type);
		glShaderSource(new_handle, 1, &shader_source_data, nullptr);
		glCompileShader(new_handle);
		GLint success = 0;
		glGetShaderiv(new_handle, GL_COMPILE_STATUS, &success);
		if(!success) {
//			char msg[1024];
//			glGetShaderInfoLog(handle, 1024, nullptr, msg);
//			std::cerr << "Shader compilation failed:\n" << msg << std::endl;
			glDeleteShader(new_handle);
			new_handle = 0;
		}

		if(new_handle != 0) {
			handle = new_handle;
			std::cerr << "[shader]   (re)loaded " << filepath.filename() << std::endl;
		}
	} catch(const boost::filesystem::filesystem_error& err) {
		std::cerr << "exception in shader reload(), code: " << err.code() << ", what(): \n" << err.what() << std::endl;
	}

	Shader(Shader&& o)
	{
		std::swap(handle,o.handle);
		std::swap(filepath, o.filepath);
		last_mod = o.last_mod;
	}

	Shader& operator=(const Shader&&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader(Shader&) = delete;

	bool valid() const
	{
		return handle != 0;
	}

	fs::path filepath;
	time_t last_mod = 0;
	GLuint handle{};
};

}

struct ShaderSet::Program
{
	Program() : handle(glCreateProgram()) { }
	~Program()
	{
		glDeleteProgram(handle);
	}

	Program(Program&& o)
	{
		std::swap(handle, o.handle);
		std::swap(m_shaders, o.m_shaders);
	}


	Program& operator=(Program&) = delete;
	Program(Program&) = delete;

	void attach_shader(Shader&& s)
	{
		if(s.valid()) {
			auto sh = s.handle;
			m_shaders.push_back(std::move(s));
			glAttachShader(handle, sh);
			//std::cerr << "attached " << m_shaders.back().filepath << std::endl;
			glDeleteShader(sh); // mark for garbage collection after detaching from program
		}
	}

	void link()
	{
		glLinkProgram(handle);
		GLint success = 0;
		glGetProgramiv(handle, GL_LINK_STATUS, &success);
		if(!success) {

			std::cerr << "[shader]  failed to link program..." << std::endl;
			//char msg[512];
			//glGetShaderInfoLog(handle, 512, nullptr, msg);
			//std::cerr << "Shader compilation failed:\n" << msg << std::endl;
			glDeleteProgram(handle);
			handle = 0;
		}
	}

	void reload()
	{
		bool should_reload = false;
		for(auto& s : m_shaders) {
			if(s.should_reload()) {
				should_reload = true;
				break;
			}
		}

		if(!should_reload)
			return;

		GLuint new_program_handle = glCreateProgram();

		size_t valid_shaders = 0;
		for(auto& s : m_shaders) {
			s.reload(true);
			if(s.valid()) {
				++valid_shaders;
			}
		}

		if(valid_shaders < m_shaders.size()) {
			std::cerr << "[shader]   only " << valid_shaders << " out of " << m_shaders.size() << " attached." << std::endl;
			return;
		}

		for(auto& s : m_shaders) {
			glAttachShader(new_program_handle, s.handle);
			glDeleteShader(s.handle);
		}

		glLinkProgram(new_program_handle);
		GLint success = 0;
		glGetProgramiv(handle, GL_LINK_STATUS, &success);
		if(!success) {
			glDeleteProgram(new_program_handle);
			new_program_handle = 0;
		} else {
			glDeleteProgram(handle);
			handle = new_program_handle;
			//std::cerr << "Reloaded program successfully!" << std::endl;
		}
	}


	bool valid() const
	{
		return handle != 0;
	}

	GLuint handle{};

private:

	std::vector<Shader> m_shaders;
};


ShaderSet::ShaderSet(std::string_view directory) : m_directory(directory)
{

}

ShaderSet::~ShaderSet()
{
	for(int i= 0; i < m_program_count; ++i)	{
		(m_programs[i])->~Program();
	}
}

void ShaderSet::check_updates()
{
	for(int i= 0; i < m_program_count; ++i)	{
		(m_programs[i])->reload();
	}
}


GLuint * ShaderSet::load_program(std::initializer_list<std::string_view> names)
{
	Program program;

	for(auto&& name : names) {
		fs::path filepath{m_directory};
		filepath /= name.data();

		if(!fs::exists(filepath)) {
			std::cerr << "[shader]   file does not exist: " << filepath << std::endl;
			return nullptr;
		}

		program.attach_shader(Shader(std::move(filepath)));
	}

	program.link();

	if(program.valid()) {
		m_programs[m_program_count] = new Program{std::move(program)};
		return &((m_programs[m_program_count++])->handle);
	}

	return nullptr;

}



