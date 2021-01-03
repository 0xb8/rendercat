#include <rendercat/uniform.hpp>
#include <rendercat/util/gl_debug.hpp>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/functions.h>
#include <zcm/type_ptr.hpp>

namespace rc {
namespace unif {

static constexpr auto map_flags = gl45core::GL_MAP_WRITE_BIT | gl45core::GL_MAP_READ_BIT | gl45core::GL_MAP_PERSISTENT_BIT;

basic_buf::basic_buf(basic_buf && other) noexcept :
        _buffer{std::exchange(other._buffer, rc::buffer_handle{})},
        _data{std::exchange(other._data, nullptr)}
{

}

basic_buf::~basic_buf()
{
	unmap();
}

basic_buf & basic_buf::operator=(basic_buf && other) noexcept
{
	unmap();
	_buffer = std::exchange(other._buffer, rc::buffer_handle{});
	_data = std::exchange(other._data, nullptr);
	return *this;
}

void basic_buf::unmap()
{
	if (_buffer && _data) {
		gl45core::glUnmapNamedBuffer(*_buffer);
	}
	_data = nullptr;
}

basic_buf::basic_buf(size_t size)
{

	gl45core::glCreateBuffers(1, _buffer.get());
	gl45core::glNamedBufferStorage(*_buffer, size, nullptr,
	                               map_flags | gl45core::GL_DYNAMIC_STORAGE_BIT);
}

void basic_buf::bind_single(uint32_t index) const
{
	gl45core::glBindBufferBase(gl45core::GL_UNIFORM_BUFFER, index, *_buffer);
}

void basic_buf::bind_multi(uint32_t index, size_t offset, size_t size) const
{
	gl45core::glBindBufferRange(gl45core::GL_UNIFORM_BUFFER, index, *_buffer, offset, size);
}

void basic_buf::map(size_t size)
{
	if (_data) return; // already mapped

	if (_buffer) {
		auto map = glMapNamedBufferRange(*_buffer, 0, size, map_flags | gl45core::GL_MAP_FLUSH_EXPLICIT_BIT);
		assert(map);
		_data = map;
	} else {
		_data = nullptr;
	}
}

void basic_buf::flush(size_t offset, size_t size)
{
	if (_data)
		gl45core::glFlushMappedNamedBufferRange(*_buffer, offset, size);
}

basic_buf::operator bool() const noexcept
{
	return static_cast<bool>(_buffer);
}


void basic_buf::set_label(std::string_view label)
{
	rcObjectLabel(_buffer, label);
}


// --------------- single uniforms ----------------


void b1(uint32_t shader, std::string_view name, bool value)
{
	b1(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

void b1(uint32_t shader, int location, bool value)
{
	gl45core::glProgramUniform1i(shader, location, (int)value);
}

void i1(uint32_t shader, int location, int value)
{
	gl45core::glProgramUniform1i(shader, location, value);
}

void i1(uint32_t shader, std::string_view name, int value)
{
	i1(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

void i2(uint32_t shader, int location, int a, int b)
{
	gl45core::glProgramUniform2i(shader, location, gl45core::GLint(a), b);
}

void i2(uint32_t shader, std::string_view name, int a, int b)
{
	i2(shader, gl45core::glGetUniformLocation(shader, name.data()), a, b);
}

void f1(uint32_t shader, int location, float value)
{
	gl45core::glProgramUniform1f(shader, location, value);
}

void f1(uint32_t shader, std::string_view name, float value)
{
	f1(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

void v2(uint32_t shader, int location, zcm::vec2 value)
{
	gl45core::glProgramUniform2fv(shader, location, 1, zcm::value_ptr(value));
}

void v2(uint32_t shader, std::string_view name, zcm::vec2 value)
{
	v2(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

void v3(uint32_t shader, int location, zcm::vec3 value)
{
	gl45core::glProgramUniform3fv(shader, location, 1, zcm::value_ptr(value));
}

void v3(uint32_t shader, std::string_view name, zcm::vec3 value)
{
	v3(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

void v4(uint32_t shader, int location, zcm::vec4 value)
{
	gl45core::glProgramUniform4fv(shader, location, 1, zcm::value_ptr(value));
}

void v4(uint32_t shader, std::string_view name, zcm::vec4 value)
{
	v4(shader, gl45core::glGetUniformLocation(shader, name.data()), value);
}

void m3(uint32_t shader, int location, const zcm::mat3 & mat)
{
	gl45core::glProgramUniformMatrix3fv(shader, location, 1, gl45core::GL_FALSE, zcm::value_ptr(mat));
}

void m3(uint32_t shader, std::string_view name, const zcm::mat3 & mat)
{
	m3(shader, gl45core::glGetUniformLocation(shader, name.data()), mat);
}

void m4(uint32_t shader, int location, const zcm::mat4 & mat)
{
	gl45core::glProgramUniformMatrix4fv(shader, location, 1, gl45core::GL_FALSE, zcm::value_ptr(mat));
}

void m4(uint32_t shader, std::string_view name, const zcm::mat4 & mat)
{
	m4(shader, gl45core::glGetUniformLocation(shader, name.data()), mat);
}


} // namespace unif
} // namespace rc
