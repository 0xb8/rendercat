#pragma once

#include <rendercat/common.hpp>
#include <string>
#include <string_view>
#include <filesystem>
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
	const std::string& name() const;
	const std::string& value() const;
private:
	std::string m_name;
	std::string m_value;
};

class ShaderSet
{
public:

	// Only single instance is allowed. MUST NOT be static or global instance.
	explicit ShaderSet(std::filesystem::path directory = path::shader);
	~ShaderSet();

	void check_updates();

	using macros_t = std::vector<ShaderMacro>;

	uint32_t* load_program(std::vector<std::filesystem::path>&& paths, macros_t&& defines = macros_t());
	bool deleteProgram(uint32_t**);

	static constexpr size_t max_programs = 16;

private:
	class Program;
	std::filesystem::path m_directory;
	Program*    m_programs[max_programs];
	unsigned    m_program_count = 0;
	unsigned    m_next_reload_index = 0;
	bool        m_dirty = true;

};

} // namespace rc
