#pragma once
#include "unique_handle.hpp"
#include <stdint.h>
#include <glbinding/gl45core/types.h>

namespace rc {

struct BufferDeleter
{
	void operator()(gl45core::GLuint) noexcept;
};

struct FrameBufferDeleter
{
	void operator()(gl45core::GLuint) noexcept;
};

struct ProgramDeleter
{
	void operator()(gl45core::GLuint) noexcept;
};

struct QueryDeleter
{
	void operator()(gl45core::GLuint) noexcept;
};

struct ShaderDeleter
{
	void operator()(gl45core::GLuint) noexcept;
};

struct TextureDeleter
{
	void operator()(gl45core::GLuint) noexcept;
};

struct VertexArrayDeleter
{
	void operator()(gl45core::GLuint) noexcept;
};

struct SyncDeleter
{
	void operator()(gl45core::GLsync) noexcept;
};

using buffer_handle       = unique_handle<gl45core::GLuint, BufferDeleter>;
using framebuffer_handle  = unique_handle<gl45core::GLuint, FrameBufferDeleter>;
using program_handle      = unique_handle<gl45core::GLuint, ProgramDeleter>;
using query_handle        = unique_handle<gl45core::GLuint, QueryDeleter>;
using shader_handle       = unique_handle<gl45core::GLuint, ShaderDeleter>;
using texture_handle      = unique_handle<gl45core::GLuint, TextureDeleter>;
using vertex_array_handle = unique_handle<gl45core::GLuint, VertexArrayDeleter>;
using sync_handle         = unique_handle<gl45core::GLsync, SyncDeleter>;

}
