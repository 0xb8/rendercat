#pragma once
#include "unique_handle.hpp"
#include <stdint.h>

namespace rc {

struct BufferDeleter
{
	void operator()(uint32_t) noexcept;
};

struct FrameBufferDeleter
{
	void operator()(uint32_t) noexcept;
};

struct ProgramDeleter
{
	void operator()(uint32_t) noexcept;
};

struct QueryDeleter
{
	void operator()(uint32_t) noexcept;
};

struct ShaderDeleter
{
	void operator()(uint32_t) noexcept;
};

struct TextureDeleter
{
	void operator()(uint32_t) noexcept;
};

struct VertexArrayDeleter
{
	void operator()(uint32_t) noexcept;
};

using buffer_handle       = unique_handle<uint32_t, BufferDeleter>;
using framebuffer_handle  = unique_handle<uint32_t, FrameBufferDeleter>;
using program_handle      = unique_handle<uint32_t, ProgramDeleter>;
using query_handle        = unique_handle<uint32_t, QueryDeleter>;
using shader_handle       = unique_handle<uint32_t, ShaderDeleter>;
using texture_handle      = unique_handle<uint32_t, TextureDeleter>;
using vertex_array_handle = unique_handle<uint32_t, VertexArrayDeleter>;

}
