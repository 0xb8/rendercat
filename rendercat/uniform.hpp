#pragma once
#include <rendercat/common.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <zcm/vec2.hpp>
#include <zcm/vec3.hpp>
#include <zcm/mat3.hpp>
#include <zcm/mat4.hpp>
#include <string_view>

namespace rc {
namespace unif {

// --- bool --------------------------------------------------------------------

void b1(uint32_t shader, int location, bool value);
void b1(uint32_t shader, std::string_view name, bool value);

// --- int --------------------------------------------------------------------

void i1(uint32_t shader, int location, int value);
void i1(uint32_t shader, std::string_view name, int value);

void i2(uint32_t shader, int location, int a, int b);
void i2(uint32_t shader, std::string_view name, int a, int b);

// --- float -------------------------------------------------------------------

void f1(uint32_t shader, int location, float value);
void f1(uint32_t shader, std::string_view name, float value);

// --- vec ---------------------------------------------------------------------

void v2(uint32_t shader, int location, zcm::vec2 value);
void v2(uint32_t shader, std::string_view name, zcm::vec2 value);

void v3(uint32_t shader, int location, zcm::vec3 value);
void v3(uint32_t shader, std::string_view name, zcm::vec3 value);

void v4(uint32_t shader, int location, zcm::vec4 value);
void v4(uint32_t shader, std::string_view name, zcm::vec4 value);

// --- mat ---------------------------------------------------------------------

void m3(uint32_t shader, int location, const zcm::mat3 &mat);
void m3(uint32_t shader, std::string_view name, const zcm::mat3 &mat);

void m4(uint32_t shader, int location, const zcm::mat4 &mat);
void m4(uint32_t shader, std::string_view name, const zcm::mat4 &mat);


struct basic_buf {
	void unmap();
	explicit operator bool() const noexcept;
	void set_label(std::string_view label);

protected:
	basic_buf(size_t size);
	~basic_buf();

	RC_DISABLE_COPY(basic_buf)

	basic_buf(basic_buf&& other) noexcept;
	basic_buf& operator=(basic_buf&& other) noexcept;

	void bind_single(uint32_t index) const;
	void bind_multi(uint32_t index, size_t offset, size_t size) const;
	void map(size_t size);
	void flush(size_t offset, size_t size);

	rc::buffer_handle _buffer;
	void* _data = nullptr;
};

template<typename T, unsigned N=1>
struct buf : public basic_buf {

	static constexpr auto _internal_size = sizeof (T) * N;

	buf() : basic_buf(_internal_size) {
		map(true);
	}

	buf(buf&& other) : basic_buf(std::move(other)), _index(other.index) { }

	buf& operator=(buf&& other) {
		basic_buf::operator=(std::move(other));
		_index = other._index;
		return *this;
	}

	T* map(bool init) {
		basic_buf::map(_internal_size);
		if (init && _data)
			new(_data) T[N]{};
		return data();
	}

	T* data() const noexcept {
		return reinterpret_cast<T*>(_data) + _index;
	}

	void bind(uint32_t index) const {
		if constexpr (N == 1) {
			basic_buf::bind_single(index);
		} else {
			basic_buf::bind_multi(index, _index * sizeof(T), sizeof(T));
		}
	}

	void flush() {
		basic_buf::flush(_index * sizeof (T), sizeof (T));
	}

	void next() {
		static_assert (N > 1, "Only allowed for N-buffering.");

		_index = (_index+1) % N;
	}

private:
	size_t _index = 0;
};

} // namespace unif
} // namespace rc
