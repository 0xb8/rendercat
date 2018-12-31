#pragma once

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

#define RC_DEFAULT_COPY(Class) \
	Class(Class&) = default; \
	Class& operator =(Class&) =  default;

#define RC_DEFAULT_COPY_NOEXCEPT(Class) \
	Class(Class&) noexcept = default; \
	Class& operator =(Class&) noexcept = default;

namespace rc {
namespace math {
	template<typename T>
	inline T percent(T value, T max, T min = T{})
	{
		//static_assert(std::is_fundamental_v<T>, "T must be fundamental type");
		if(value == T{} || max == T{} || max == min) return T{};
		return static_cast<double>(value-min) * 100.0 / max - min;
	}
} // namespace math

namespace path {
	constexpr char shader[] = "shaders/";
	namespace asset {
		constexpr char cubemap[] = "assets/materials/cubemaps/";
		constexpr char model[] = "assets/models/";
		constexpr char model_material[] = "assets/materials/models/";
	}
} // namespace path

} // namespace rc


