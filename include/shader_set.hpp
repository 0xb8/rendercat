#pragma once

#include <GL/glew.h>
#include <array>
#include <string>
#include <string_view>
#include <glm/glm.hpp>



namespace unif {

inline void b1(GLuint shader, const std::string_view name, bool value)
{
	glUniform1i(glGetUniformLocation(shader, name.data()), (int)value);
}

inline void i1(GLuint shader, const std::string_view name, int value)
{
	glUniform1i(glGetUniformLocation(shader, name.data()), value);
}

inline void f1(GLuint shader, const std::string_view name, float value)
{
	glUniform1f(glGetUniformLocation(shader, name.data()), value);
}

inline void v2(GLuint shader, const std::string_view name, const glm::vec2 &value)
{
	glUniform2fv(glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v2(GLuint shader, const std::string_view name, float x, float y)
{
	glUniform2f(glGetUniformLocation(shader, name.data()), x, y);
}

inline void v3(GLuint shader, const std::string_view name, const glm::vec3 &value)
{
	glUniform3fv(glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v3(GLuint shader, const std::string_view name, float x, float y, float z)
{
	glUniform3f(glGetUniformLocation(shader, name.data()), x, y, z);
}

inline void v4(GLuint shader, const std::string_view name, const glm::vec4 &value)
{
	glUniform4fv(glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v4(GLuint shader, const std::string_view name, float x, float y, float z, float w)
{
	glUniform4f(glGetUniformLocation(shader, name.data()), x, y, z, w);
}

inline void m2(GLuint shader, const std::string_view name, const glm::mat2 &mat)
{
	glUniformMatrix2fv(glGetUniformLocation(shader, name.data()), 1, GL_FALSE, &mat[0][0]);
}

inline void m3(GLuint shader, const std::string_view name, const glm::mat3 &mat)
{
	glUniformMatrix3fv(glGetUniformLocation(shader, name.data()), 1, GL_FALSE, &mat[0][0]);
}

inline void m4(GLuint shader, const std::string_view name, const glm::mat4 &mat)
{
	glUniformMatrix4fv(glGetUniformLocation(shader, name.data()), 1, GL_FALSE, &mat[0][0]);
}

}

class ProgramPlaceholder
{
	uint8_t _filler[32];
};



class ShaderSet
{
public:

	explicit ShaderSet(std::string_view directory = "shaders");
	~ShaderSet();

	void check_updates();

	GLuint* load_program(std::initializer_list<std::string_view>);

	static constexpr size_t max_programs = 16;

private:


	std::string		m_version;
	std::string		m_directory;
	ProgramPlaceholder	m_programs_storage[max_programs];
	int			m_program_count = 0;

};

