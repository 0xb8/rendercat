#pragma once
#include <rendercat/common.hpp>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/functions.h>
#include <string_view>

namespace unif {

// --- bool --------------------------------------------------------------------

inline void b1(gl45core::GLuint shader, const std::string_view name, bool value)
{
	gl45core::glProgramUniform1i(shader, gl45core::glGetUniformLocation(shader, name.data()), (int)value);
}

// --- int --------------------------------------------------------------------

inline void i1(gl45core::GLuint shader, gl45core::GLint location, int value)
{
	gl45core::glProgramUniform1i(shader, location, value);
}

inline void i1(gl45core::GLuint shader, const std::string_view name, int value)
{
	i1(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

inline void i2(gl45core::GLuint shader, gl45core::GLint location, int a, int b)
{
	gl45core::glProgramUniform2i(shader, location, gl45core::GLint(a), b);
}

inline void i2(gl45core::GLuint shader, const std::string_view name, int a, int b)
{
	i2(shader, gl45core::glGetUniformLocation(shader, name.data()), a, b);
}

// --- float -------------------------------------------------------------------

inline void f1(gl45core::GLuint shader, gl45core::GLint location, float value)
{
	gl45core::glProgramUniform1f(shader, location, value);
}

inline void f1(gl45core::GLuint shader, const std::string_view name, float value)
{
	f1(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

// --- vec ---------------------------------------------------------------------

inline void v2(gl45core::GLuint shader, gl45core::GLint location, const glm::vec2 &value)
{
	gl45core::glProgramUniform2fv(shader, location, 1, &value[0]);
}

inline void v2(gl45core::GLuint shader, const std::string_view name, const glm::vec2 &value)
{
	v2(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}


inline void v3(gl45core::GLuint shader, gl45core::GLint location, const glm::vec3 &value)
{
	gl45core::glProgramUniform3fv(shader, location, 1, &value[0]);
}

inline void v3(gl45core::GLuint shader, const std::string_view name, const glm::vec3 &value)
{
	v3(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}


inline void v4(gl45core::GLuint shader, gl45core::GLint location, const glm::vec4 &value)
{
	gl45core::glProgramUniform4fv(shader, location, 1, &value[0]);
}

inline void v4(gl45core::GLuint shader, const std::string_view name, const glm::vec4 &value)
{
	v4(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

// --- mat ---------------------------------------------------------------------

inline void m3(gl45core::GLuint shader, gl45core::GLint location, const glm::mat3 &mat)
{
	gl45core::glProgramUniformMatrix3fv(shader, location, 1, gl45core::GL_FALSE, &mat[0][0]);
}

inline void m3(gl45core::GLuint shader, const std::string_view name, const glm::mat3 &mat)
{
	m3(shader, gl45core::glGetUniformLocation(shader, name.data()), mat);
}

inline void m4(gl45core::GLuint shader, gl45core::GLint location, const glm::mat4 &mat)
{
	gl45core::glProgramUniformMatrix4fv(shader, location, 1, gl45core::GL_FALSE, &mat[0][0]);
}

inline void m4(gl45core::GLuint shader, const std::string_view name, const glm::mat4 &mat)
{
	m4(shader, gl45core::glGetUniformLocation(shader, name.data()), mat);
}

}
