#pragma once
#include <rendercat/common.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <string_view>

namespace rc {

class Cubemap
{
	buffer_handle        m_vbo;
	vertex_array_handle  m_vao;
	texture_handle       m_cubemap;

	RC_DISABLE_COPY(Cubemap)

public:
	Cubemap() noexcept;
	~Cubemap()  = default;

	Cubemap(Cubemap&& o) noexcept = default;
	Cubemap& operator=(Cubemap&& o) noexcept = default;

	void load_textures(std::string_view basedir);
	void draw(uint32_t shader, const glm::mat4& view, const glm::mat4& projection) noexcept;
};

} // namespace rc
