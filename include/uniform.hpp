#pragma once
#include <GL/glew.h>
#include <common.hpp>
#include <string_view>

namespace unif {

inline void b1(GLuint shader, const std::string_view name, bool value)
{
	glProgramUniform1i(shader, glGetUniformLocation(shader, name.data()), (int)value);
}

inline void i1(GLuint shader, const std::string_view name, int value)
{
	glProgramUniform1i(shader, glGetUniformLocation(shader, name.data()), value);
}

inline void i2(GLuint shader, const std::string_view name, int x, int y)
{
	glProgramUniform2i(shader, glGetUniformLocation(shader, name.data()), x, y);
}

inline void f1(GLuint shader, const std::string_view name, float value)
{
	glProgramUniform1f(shader, glGetUniformLocation(shader, name.data()), value);
}

inline void v2(GLuint shader, const std::string_view name, const glm::vec2 &value)
{
	glProgramUniform2fv(shader, glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v2(GLuint shader, const std::string_view name, float x, float y)
{
	glProgramUniform2f(shader, glGetUniformLocation(shader, name.data()), x, y);
}

inline void v3(GLuint shader, const std::string_view name, const glm::vec3 &value)
{
	glProgramUniform3fv(shader, glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v3(GLuint shader, const std::string_view name, float x, float y, float z)
{
	glProgramUniform3f(shader, glGetUniformLocation(shader, name.data()), x, y, z);
}

inline void v4(GLuint shader, const std::string_view name, const glm::vec4 &value)
{
	glProgramUniform4fv(shader, glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v4(GLuint shader, const std::string_view name, float x, float y, float z, float w)
{
	glProgramUniform4f(shader, glGetUniformLocation(shader, name.data()), x, y, z, w);
}

inline void m2(GLuint shader, const std::string_view name, const glm::mat2 &mat)
{
	glProgramUniformMatrix2fv(shader, glGetUniformLocation(shader, name.data()), 1, GL_FALSE, &mat[0][0]);
}

inline void m3(GLuint shader, const std::string_view name, const glm::mat3 &mat)
{
	glProgramUniformMatrix3fv(shader, glGetUniformLocation(shader, name.data()), 1, GL_FALSE, &mat[0][0]);
}

inline void m4(GLuint shader, const std::string_view name, const glm::mat4 &mat)
{
	glProgramUniformMatrix4fv(shader, glGetUniformLocation(shader, name.data()), 1, GL_FALSE, &mat[0][0]);
}

}
