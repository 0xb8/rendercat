#pragma once
#include <rendercat/common.hpp>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/functions.h>
#include <string_view>

namespace unif {

inline void b1(gl45core::GLuint shader, const std::string_view name, bool value)
{
	gl45core::glProgramUniform1i(shader, gl45core::glGetUniformLocation(shader, name.data()), (int)value);
}

inline void i1(gl45core::GLuint shader, const std::string_view name, int value)
{
	gl45core::glProgramUniform1i(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

inline void i2(gl45core::GLuint shader, const std::string_view name, int x, int y)
{
	gl45core::glProgramUniform2i(shader, gl45core::glGetUniformLocation(shader, name.data()), x, y);
}

inline void f1(gl45core::GLuint shader, const std::string_view name, float value)
{
	gl45core::glProgramUniform1f(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

inline void v2(gl45core::GLuint shader, const std::string_view name, const glm::vec2 &value)
{
	gl45core::glProgramUniform2fv(shader, gl45core::glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v2(gl45core::GLuint shader, const std::string_view name, float x, float y)
{
	gl45core::glProgramUniform2f(shader, gl45core::glGetUniformLocation(shader, name.data()), x, y);
}

inline void v3(gl45core::GLuint shader, const std::string_view name, const glm::vec3 &value)
{
	gl45core::glProgramUniform3fv(shader, gl45core::glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v3(gl45core::GLuint shader, const std::string_view name, float x, float y, float z)
{
	gl45core::glProgramUniform3f(shader, gl45core::glGetUniformLocation(shader, name.data()), x, y, z);
}

inline void v4(gl45core::GLuint shader, const std::string_view name, const glm::vec4 &value)
{
	gl45core::glProgramUniform4fv(shader, gl45core::glGetUniformLocation(shader, name.data()), 1, &value[0]);
}
inline void v4(gl45core::GLuint shader, const std::string_view name, float x, float y, float z, float w)
{
	gl45core::glProgramUniform4f(shader, gl45core::glGetUniformLocation(shader, name.data()), x, y, z, w);
}

inline void m2(gl45core::GLuint shader, const std::string_view name, const glm::mat2 &mat)
{
	gl45core::glProgramUniformMatrix2fv(shader, gl45core::glGetUniformLocation(shader, name.data()), 1, gl45core::GL_FALSE, &mat[0][0]);
}

inline void m3(gl45core::GLuint shader, const std::string_view name, const glm::mat3 &mat)
{
	gl45core::glProgramUniformMatrix3fv(shader, gl45core::glGetUniformLocation(shader, name.data()), 1, gl45core::GL_FALSE, &mat[0][0]);
}

inline void m4(gl45core::GLuint shader, const std::string_view name, const glm::mat4 &mat)
{
	gl45core::glProgramUniformMatrix4fv(shader, gl45core::glGetUniformLocation(shader, name.data()), 1, gl45core::GL_FALSE, &mat[0][0]);
}

}
