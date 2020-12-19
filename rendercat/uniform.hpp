#pragma once
#include <rendercat/common.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/functions.h>
#include <string_view>
#include <zcm/type_ptr.hpp>

namespace rc {
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

inline void v2(gl45core::GLuint shader, gl45core::GLint location, const zcm::vec2 &value)
{
	gl45core::glProgramUniform2fv(shader, location, 1, zcm::value_ptr(value));
}

inline void v2(gl45core::GLuint shader, const std::string_view name, const zcm::vec2 &value)
{
	v2(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}


inline void v3(gl45core::GLuint shader, gl45core::GLint location, const zcm::vec3 &value)
{
	gl45core::glProgramUniform3fv(shader, location, 1, zcm::value_ptr(value));
}

inline void v3(gl45core::GLuint shader, const std::string_view name, const zcm::vec3 &value)
{
	v3(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}


inline void v4(gl45core::GLuint shader, gl45core::GLint location, const zcm::vec4 &value)
{
	gl45core::glProgramUniform4fv(shader, location, 1, zcm::value_ptr(value));
}

inline void v4(gl45core::GLuint shader, const std::string_view name, const zcm::vec4 &value)
{
	v4(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

// --- mat ---------------------------------------------------------------------

inline void m3(gl45core::GLuint shader, gl45core::GLint location, const zcm::mat3 &mat)
{
	gl45core::glProgramUniformMatrix3fv(shader, location, 1, gl45core::GL_FALSE, zcm::value_ptr(mat));
}

inline void m3(gl45core::GLuint shader, const std::string_view name, const zcm::mat3 &mat)
{
	m3(shader, gl45core::glGetUniformLocation(shader, name.data()), mat);
}

inline void m4(gl45core::GLuint shader, gl45core::GLint location, const zcm::mat4 &mat)
{
	gl45core::glProgramUniformMatrix4fv(shader, location, 1, gl45core::GL_FALSE, zcm::value_ptr(mat));
}

inline void m4(gl45core::GLuint shader, const std::string_view name, const zcm::mat4 &mat)
{
	m4(shader, gl45core::glGetUniformLocation(shader, name.data()), mat);
}

template<typename T, gl45core::GLenum BindBufferType, unsigned N=1>
struct buf {

	static constexpr auto map_flags = gl45core::GL_MAP_WRITE_BIT | gl45core::GL_MAP_READ_BIT | gl45core::GL_MAP_PERSISTENT_BIT;
	static constexpr auto _internal_size = sizeof (T) * N;

	buf() {
		gl45core::glCreateBuffers(1, _buffer.get());
		gl45core::glNamedBufferStorage(*_buffer, _internal_size, nullptr,
		                               map_flags | gl45core::GL_DYNAMIC_STORAGE_BIT);
		map(true);
	}

	~buf() {
		unmap();
	}

	RC_DISABLE_COPY(buf);

	buf(buf&& other) noexcept :
		_buffer{std::exchange(other._buffer, rc::buffer_handle{})},
		_data{std::exchange(other._data, nullptr)}
	{

	}

	buf& operator=(buf&& other) noexcept {
		unmap();
		_buffer = std::exchange(other._buffer, rc::buffer_handle{});
		_data = std::exchange(other._data, nullptr);
		return *this;
	}

	T* map(bool init)
	{
		if (_data) return data(); // already mapped

		if (_buffer) {
			auto map = glMapNamedBufferRange(*_buffer, 0, _internal_size, map_flags | gl45core::GL_MAP_FLUSH_EXPLICIT_BIT);
			assert(map);
			if (init)
				_data = new(map) T[N]{};
			else
				_data = reinterpret_cast<T*>(map);
		} else {
			_data = nullptr;
		}
		return data();
	}

	T* data() const noexcept {
		return _data + _index;
	}

	void bind(uint32_t index) const {
		if constexpr(N == 1) {
			gl45core::glBindBufferBase(BindBufferType, index, *_buffer);
		} else {
			gl45core::glBindBufferRange(BindBufferType, index, *_buffer, _index * sizeof (T), sizeof (T));
		}
	}

	void unmap()
	{
		if (_buffer && _data) {
			gl45core::glUnmapNamedBuffer(*_buffer);
		}
		_data = nullptr;
	}

	void flush()
	{
		if (_data)
			gl45core::glFlushMappedNamedBufferRange(*_buffer, _index * sizeof (T), sizeof (T));
	}

	void next() {
		static_assert (N > 1, "Only allowed for N-buffering.");

		_index = (_index+1) % N;
	}

	explicit operator bool() const noexcept {
		return static_cast<bool>(_buffer);
	}

private:
	rc::buffer_handle _buffer;
	T* _data = nullptr;
	size_t _index = 0;
};

} // namespace unif
} // namespace rc
