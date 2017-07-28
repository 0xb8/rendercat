#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>
#include <glm/gtx/string_cast.hpp>

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef short GLshort;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned long GLulong;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

#if __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif


inline std::ostream& operator<<(std::ostream& out, const glm::vec2& g)
{
    return out << glm::to_string(g);
}
inline std::ostream& operator<<(std::ostream& out, const glm::vec3& g)
{
    return out << glm::to_string(g);
}
inline std::ostream& operator<<(std::ostream& out, const glm::vec4& g)
{
    return out << glm::to_string(g);
}

namespace m {

	inline bool saturated(float f)
	{
		return f >= 0.0f && f <= 1.0f;
	}
	inline bool saturated(double f)
	{
		return f >= 0.0f && f <= 1.0f;
	}
	inline bool saturated(glm::vec2 v)
	{
		return saturated(v[0]) && saturated(v[1]);
	}

	inline bool saturated(glm::vec3 v)
	{
		return saturated(v[0]) && saturated(v[1]) && saturated(v[2]);
	}

	inline bool saturated(glm::vec4 v)
	{
		return saturated(v[0]) && saturated(v[1]) && saturated(v[2]) && saturated(v[3]);
	}
}

namespace mc {

inline constexpr double M_E	= 2.7182818284590452354;
inline constexpr double M_LOG2E	= 1.4426950408889634074;
inline constexpr double M_LOG10E= 0.43429448190325182765;
inline constexpr double M_LN2	= 0.69314718055994530942;
inline constexpr double M_LN10	= 2.30258509299404568402;
inline constexpr double M_PI	= 3.14159265358979323846;
inline constexpr double M_PI_2	= 1.57079632679489661923;
inline constexpr double M_PI_4	= 0.78539816339744830962;
inline constexpr double M_1_PI	= 0.31830988618379067154;
inline constexpr double M_2_PI	= 0.63661977236758134308;
inline constexpr double M_2_SQRTPI = 1.12837916709551257390;
inline constexpr double M_SQRT2	= 1.41421356237309504880;
inline constexpr double M_SQRT1_2 = 0.70710678118654752440;

}
