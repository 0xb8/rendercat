#pragma once
#include <type_traits>

namespace rc {

template<typename T, typename Deleter>
class unique_handle
{
	T m_handle{};

	unique_handle(const unique_handle&) = delete;
	unique_handle& operator =(const unique_handle&) = delete;

public:
	unique_handle() noexcept = default;
	explicit unique_handle(const T& handle) noexcept :
		m_handle(handle)
	{
		static_assert(std::is_fundamental_v<T> || std::is_pointer_v<T>, "T must be fundamental type or pointer");
	}

	~unique_handle() noexcept {
		static_assert(std::is_nothrow_invocable_v<Deleter, T>, "deleter must take T as sole parameter and be noexcept");
		static_assert(std::is_same_v<std::invoke_result_t<Deleter, T>, void>, "deleter must return void");
		reset();
	}

	template<typename D2>
	unique_handle(unique_handle<T, D2>&& other) noexcept : m_handle(other.release())
	{
		static_assert(std::is_same_v<Deleter, D2>, "both handles must have same deleter type");
	}

	template<typename D2>
	unique_handle& operator =(unique_handle<T, D2>&& other) noexcept {
		static_assert(std::is_same_v<Deleter, D2>, "both handles must have same deleter type");
		if(this != &other) {
			this->reset(other.m_handle);
			other.m_handle = 0;
		}
		return *this;
	}

	T release() noexcept
	{
		auto r = m_handle;
		m_handle = 0;
		return r;
	}

	void reset(T new_handle = T{}) noexcept
	{
		if(m_handle) {
			Deleter d; d(m_handle);
		}
		m_handle = new_handle;
	}

	void swap(unique_handle& other) noexcept
	{
		std::swap(m_handle, other.m_handle);
	}

	explicit operator bool() const noexcept
	{
		return m_handle != 0;
	}

	std::add_lvalue_reference_t<T> operator*() noexcept
	{
		return m_handle;
	}

	T operator *() const noexcept
	{
		return m_handle;
	}

	T* get() noexcept
	{
		return &m_handle;
	}

#define DECL_COMPARISON(Op, Friend) \
	template<typename T1, typename D1, typename T2, typename D2> \
	Friend inline bool operator Op(const unique_handle<T1,D1>& a, const unique_handle<T2,D2>& b) noexcept

	DECL_COMPARISON(==, friend);
	DECL_COMPARISON(!=, friend);
	DECL_COMPARISON(<=, friend);
	DECL_COMPARISON(>=, friend);
	DECL_COMPARISON(<, friend);
	DECL_COMPARISON(>, friend);
};

#define IMPL_COMPARISON(Op)                                                             \
DECL_COMPARISON(Op,) {                                                                  \
	static_assert(std::is_same_v<T1,T2>, "handles must have same underlying type"); \
	static_assert(std::is_same_v<D1,D2>, "handles must have same deleter");         \
	return a.m_handle Op b.m_handle; }

IMPL_COMPARISON(==)
IMPL_COMPARISON(!=)
IMPL_COMPARISON(<=)
IMPL_COMPARISON(>=)
IMPL_COMPARISON(<)
IMPL_COMPARISON(>)

#undef IMPL_COMPARISON
#undef DECL_COMPARISON

}
