#pragma once

#include <rendercat/common.hpp>
#include <string>

namespace rc {

class ShaderSet
{
public:

	explicit ShaderSet(std::string_view directory = path::shader);
	~ShaderSet();

	bool check_updates();

	uint32_t* load_program(std::initializer_list<std::string_view>);

	static constexpr size_t max_programs = 16;

private:
	class Program;
	std::string	m_directory;
	Program*	m_programs[max_programs];
	unsigned	m_program_count = 0;

};

} // namespace rc
