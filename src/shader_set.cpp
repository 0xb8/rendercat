#include <shader_set.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>

#define VERT_SHADER_EXT ".vert"
#define FRAG_SHADER_EXT ".frag"


namespace fs = boost::filesystem;



struct Shader
{
	Shader(const char* source, int type);
	~Shader();

	Shader(Shader&& o);
	Shader(Shader&) = delete;

	bool is_valid() const;

	GLuint handle{};
	std::string filename;
	uint64_t last_mod;
};

struct Program
{
	Program();
	~Program();

	Program(Program&& o);
	Program(Program&) = delete;

	void attach_shader(Shader&&);
	void link();

	bool is_valid() const;

	GLuint handle{};

	bool operator <(const Program& o) const {return handle < o.handle;}
private:

	std::vector<Shader> m_shaders;
};


ShaderSet::ShaderSet(const char * directory) : m_directory(directory) { }
ShaderSet::~ShaderSet() { }



Shader::Shader(const char *source, int type)
{
	handle = glCreateShader(type);
	glShaderSource(handle, 1, &source, nullptr);
	glCompileShader(handle);
	GLint success = 0;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
	if(!success) {
		char msg[1024];
		glGetShaderInfoLog(handle, 1024, nullptr, msg);
		std::cerr << "Shader compilation failed:\n" << msg << std::endl;
		glDeleteShader(handle);
		handle = 0;
	}
}

Shader::~Shader()
{
	glDeleteShader(handle);
}

Shader::Shader(Shader && o)
{
	std::swap(handle,o.handle);
}

bool Shader::is_valid() const
{
	return handle != 0;
}

Program::Program() : handle(glCreateProgram()) { }

Program::~Program()
{
	glDeleteProgram(handle);
}
Program::Program(Program && o)
{
	std::swap(handle, o.handle);
}

void Program::attach_shader(Shader && s)
{
	if(s.is_valid()) {
		m_shaders.push_back(std::move(s));
		glAttachShader(handle, m_shaders.back().handle);
	}
}

void Program::link()
{
	glLinkProgram(handle);
	GLint success = 0;
	glGetProgramiv(handle, GL_LINK_STATUS, &success);
	if(!success) {
		char msg[512];
		glGetShaderInfoLog(handle, 512, nullptr, msg);
		std::cerr << "Shader compilation failed:\n" << msg << std::endl;
		glDeleteProgram(handle);
		handle = 0;
	}
	m_shaders.clear();
}

bool Program::is_valid() const
{
	return handle != 0;
}





GLuint * ShaderSet::load_program(std::initializer_list<const char *> names)
{

	Program program;


	for(auto name : names) {
		fs::path filepath{m_directory};
		filepath /= name;

		if(!fs::exists(filepath)) {
			std::cerr << "Shader file does not exist: " << name << std::endl;
			return nullptr;
		}

		fs::ifstream sh_fstream;
		sh_fstream.open(filepath);
		if(!sh_fstream.is_open()) {
			std::cerr << "Cannot open shader file: " << name << std::endl;
			return nullptr;
		}

		auto ext = filepath.extension().string();

		int shader_type = 0;
		if(ext == VERT_SHADER_EXT) {
			shader_type = GL_VERTEX_SHADER;
		}
		else if(ext == FRAG_SHADER_EXT) {
			shader_type = GL_FRAGMENT_SHADER;
		}
		else {
			std::cerr << "Unknown shader type: " << ext << std::endl;
			return nullptr;
		}

		std::ostringstream oss{};
		oss << sh_fstream.rdbuf();
		auto shader_source = oss.str();
		auto shader_source_data = shader_source.c_str();

		Shader shader(shader_source_data, shader_type);
		program.attach_shader(std::move(shader));
	}

	program.link();

	if(program.is_valid()) {
		auto res = m_programs.insert(std::move(program));
		if(res.second) {
			return const_cast<GLuint*>(&(res.first->handle));
		}
	}

	return nullptr;

}



