#pragma once
#include <GL/glew.h>
#include <common.hpp>
#include <string_view>

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
