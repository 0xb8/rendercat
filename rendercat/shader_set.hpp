#pragma once

#include <rendercat/common.hpp>
#include <string>
#include <vector>

namespace rc {

class ShaderMacro {
public:
	ShaderMacro(std::string_view name, std::string_view value = std::string_view());
	ShaderMacro(std::string_view name, int value);
	ShaderMacro(std::string_view name, unsigned value);
	ShaderMacro(std::string_view name, float value);
	ShaderMacro(std::string_view name, bool value);
	ShaderMacro(std::string_view name, double value);

	std::string get_define_string() const;
private:
	std::string m_name;
	std::string m_value;
};

class ShaderSet
{
public:

	explicit ShaderSet(std::string_view directory = path::shader);
	~ShaderSet();

	bool check_updates();

	using macros_t = std::vector<ShaderMacro>;

	uint32_t* load_program(std::vector<std::string>&& paths, macros_t&& defines = macros_t());
	bool deleteProgram(uint32_t**);

	static constexpr size_t max_programs = 16;

private:
	class Program;
	std::string	m_directory;
	Program*	m_programs[max_programs];
	unsigned	m_program_count = 0;

};

} // namespace rc
