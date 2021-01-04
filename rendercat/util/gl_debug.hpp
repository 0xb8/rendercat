#pragma once
#include <rendercat/common.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding/gl45core/enum.h>
#include <string_view>
#include <type_traits>

#define RC_DEBUG_GROUP(...) const auto _rc_debug_group = rc::rc_debug_group(__VA_ARGS__)

namespace rc {
namespace detail {

	template<typename D>
	constexpr gl45core::GLenum kind_from_deleter()
	{
		if constexpr(std::is_same_v<D, BufferDeleter>) {
			return gl45core::GL_BUFFER;
		} else if constexpr(std::is_same_v<D, FrameBufferDeleter>) {
			return gl45core::GL_FRAMEBUFFER;
		} else if constexpr(std::is_same_v<D, ProgramDeleter>) {
			return gl45core::GL_PROGRAM;
		} else if constexpr(std::is_same_v<D, QueryDeleter>) {
			return gl45core::GL_QUERY;
		} else if constexpr(std::is_same_v<D, ShaderDeleter>) {
			return gl45core::GL_SHADER;
		} else if constexpr(std::is_same_v<D, TextureDeleter>) {
			return gl45core::GL_TEXTURE;
		} else  if constexpr(std::is_same_v<D, VertexArrayDeleter>) {
			return gl45core::GL_VERTEX_ARRAY;
		} else {
			// type-dependent false for static assert to work properly
			static_assert (std::is_same_v<D*, void>, "Unknown GL handle type");
		}
	}

}

template<size_t N>
void rcObjectLabel(gl45core::GLenum kind, gl45core::GLuint object, const char (&label)[N])
{
	gl45core::glObjectLabel(kind, object, N, label);
}

template<typename T, typename D, size_t N>
void rcObjectLabel(const unique_handle<T,D>& handle, const char (&label)[N])
{
	if (handle) {
		constexpr auto kind = detail::kind_from_deleter<D>();
		rcObjectLabel(kind, *handle, label);
	} else {
		assert(false && "Null handle!");
	}
}

inline void rcObjectLabel(gl45core::GLenum kind, gl45core::GLuint object, std::string_view label)
{
	gl45core::glObjectLabel(kind, object, label.size(), label.data());
}

template<typename T, typename D>
void rcObjectLabel(const unique_handle<T,D>& handle, std::string_view label)
{
	if (handle) {
		constexpr auto kind = detail::kind_from_deleter<D>();
		rcObjectLabel(kind, *handle, label);
	} else {
		assert(false && "Null handle!");
	}
}

inline std::string rcGetObjectLabel(gl45core::GLenum kind, gl45core::GLuint object)
{
	std::string buf;
	gl45core::GLsizei length = 0;
	gl45core::glGetObjectLabel(kind, object, 0, &length, nullptr);
	buf.resize(length + 4);
	gl45core::glGetObjectLabel(kind, object, buf.size(), &length, buf.data());
	buf.resize(length);
	return buf;
}

template<typename T, typename D>
inline std::string rcGetObjectLabel(const unique_handle<T,D>& handle)
{
	if (handle) {
		constexpr auto kind = detail::kind_from_deleter<D>();
		return rcGetObjectLabel(kind, *handle);
	}
	assert(false && "Null handle!");
	return "null handle";
}

struct rc_debug_group {

	static constexpr gl45core::GLenum source = gl45core::GL_DEBUG_SOURCE_APPLICATION;
	static constexpr uint32_t debug_group_id = 0x325fab8;

	template<size_t N>
	explicit rc_debug_group(const char (&label)[N], uint32_t id = debug_group_id)
	{
		gl45core::glPushDebugGroup(source, id, N, label);
	}
	explicit rc_debug_group(std::string_view label, uint32_t id = debug_group_id)
	{
		gl45core::glPushDebugGroup(source, id, label.size(), label.data());
	}
	~rc_debug_group() {
		gl45core::glPopDebugGroup();
	}

	RC_DISABLE_COPY(rc_debug_group)
	RC_DISABLE_MOVE(rc_debug_group)
};

} // namespace rc
