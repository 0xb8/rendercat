#pragma once

#include <GL/glew.h>
#include <set>
#include <vector>
#include <string>

class Program;

class ShaderSet
{
public:

	explicit ShaderSet(const char* directory = "shaders");
	~ShaderSet();

	GLuint* load_program(std::initializer_list<const char*>);


private:

	std::string		m_version;
	std::set<Program>	m_programs;
	const char*		m_directory;

};

