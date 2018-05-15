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

public:
	Cubemap();
	~Cubemap() = default;

	RC_DEFAULT_MOVE_NOEXCEPT(Cubemap)
	RC_DISABLE_COPY(Cubemap)

	void load_textures(std::string_view basedir);
	void draw(uint32_t shader, const glm::mat4& view, const glm::mat4& projection) noexcept;
};

} // namespace rc
