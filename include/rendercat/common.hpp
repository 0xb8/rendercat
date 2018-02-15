#pragma once

#define GLM_FORCE_CXX14
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstddef>
#include <limits>
#include <glm/gtx/string_cast.hpp>

#if __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define unreachable()   __builtin_unreachable()
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define unreachable() do { } while(0)
#endif

#define RC_DISABLE_COPY(Class) \
	Class(const Class &) = delete;\
	Class &operator=(const Class &) = delete;

#define RC_DISABLE_MOVE(Class) \
	Class(Class &&) = delete;\
	Class &operator=(Class &&) = delete;

#define RC_DEFAULT_MOVE(Class) \
	Class(Class&&) = default; \
	Class& operator =(Class&&) =  default;

#define RC_DEFAULT_MOVE_NOEXCEPT(Class) \
	Class(Class&&) noexcept = default; \
	Class& operator =(Class&&) noexcept = default;

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

namespace rc {
namespace math {
	template<typename T>
	inline constexpr bool within_closed_interval(T val, T min, T max)
	{
		static_assert(std::is_fundamental_v<T>, "T must be fundamental type");
		return val >= min && val <= max;
	}
	inline bool within_closed_interval(glm::vec2 v, glm::vec2 min, glm::vec2 max)
	{
		assert(min != max);
		return within_closed_interval(v[0], min[0], max[0]) && within_closed_interval(v[1], min[1], max[1]);
	}

	inline bool within_closed_interval(glm::vec3 v, glm::vec3 min, glm::vec3 max)
	{
		assert(min != max);
		return within_closed_interval(v[0], min[0], max[0])
			&& within_closed_interval(v[1], min[1], max[1])
			&& within_closed_interval(v[2], min[2], max[2]);
	}

	inline bool within_closed_interval(glm::vec4 v, glm::vec4 min, glm::vec4 max)
	{
		assert(min != max);
		return within_closed_interval(v[0], min[0], max[0])
			&& within_closed_interval(v[1], min[1], max[1])
			&& within_closed_interval(v[2], min[2], max[2])
			&& within_closed_interval(v[3], min[3], max[3]);
	}

	template<typename T>
	inline T percent(T value, T max, T min = T{})
	{
		static_assert(std::is_fundamental_v<T>, "T must be fundamental type");
		if(value == T{} || max == T{} || max == min) return T{};
		return static_cast<double>(value-min) * 100.0 / max - min;
	}
} // namespace math

namespace mathconst {
	inline constexpr double e	= 2.7182818284590452354;
	inline constexpr double log2e	= 1.4426950408889634074;
	inline constexpr double log10e	= 0.43429448190325182765;
	inline constexpr double ln2	= 0.69314718055994530942;
	inline constexpr double ln10	= 2.30258509299404568402;
	inline constexpr double pi	= 3.14159265358979323846;
	inline constexpr double pi_2	= 1.57079632679489661923;
	inline constexpr double pi_4	= 0.78539816339744830962;
	inline constexpr double one_over_pi	= 0.31830988618379067154;
	inline constexpr double two_over_pi	= 0.63661977236758134308;
	inline constexpr double two_over_sqrt_pi = 1.12837916709551257390;
	inline constexpr double sqrt2	= 1.41421356237309504880;
	inline constexpr double sqrt_1_2 = 0.70710678118654752440;
} // namespace mathconst

namespace path {
	constexpr char shader[] = "shaders/";
	namespace asset {
		constexpr char cubemap[] = "assets/materials/cubemaps/";
		constexpr char model[] = "assets/models/";
		constexpr char model_material[] = "assets/materials/models/";
	}
} // namespace path

} // namespace rc


