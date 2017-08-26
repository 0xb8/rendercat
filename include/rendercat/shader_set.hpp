#pragma once

#include <rendercat/common.hpp>
#include <string>

class ShaderSet
{
public:

	explicit ShaderSet(std::string_view directory = "shaders");
	~ShaderSet();

	void check_updates();

	GLuint* load_program(std::initializer_list<std::string_view>);

	static constexpr size_t max_programs = 16;

private:
	class Program;
	std::string	m_directory;
	Program*	m_programs[max_programs];
	int		m_program_count = 0;

};

