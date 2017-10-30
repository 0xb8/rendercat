#include <rendercat/util/gl_unique_handle.hpp>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;

void rc::BufferDeleter::operator()(uint32_t h) noexcept
{
	glDeleteBuffers(1, &h);
}

void rc::FrameBufferDeleter::operator()(uint32_t h) noexcept
{
	glDeleteFramebuffers(1, &h);
}

void rc::ProgramDeleter::operator()(uint32_t h) noexcept
{
	glDeleteProgram(h);
}

void rc::QueryDeleter::operator()(uint32_t h) noexcept
{
	glDeleteQueries(1, &h);
}

void rc::ShaderDeleter::operator()(uint32_t h) noexcept
{
	glDeleteShader(h);
}

void rc::TextureDeleter::operator()(uint32_t h) noexcept
{
	glDeleteTextures(1, &h);
}

void rc::VertexArrayDeleter::operator()(uint32_t h) noexcept
{
	glDeleteVertexArrays(1, &h);
}
