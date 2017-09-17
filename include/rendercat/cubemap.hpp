#pragma once
#include <rendercat/common.hpp>

class Cubemap
{
	uint32_t m_vbo = 0;
	uint32_t m_vao = 0;
	uint32_t m_cubemap = 0;
	uint32_t m_envmap = 0;

public:

	Cubemap() noexcept;
	~Cubemap() noexcept;

	Cubemap(Cubemap&) = delete;
	Cubemap& operator=(Cubemap&) = delete;

	Cubemap(Cubemap&& o) noexcept
	{
		this->operator=(std::move(o));
	}

	Cubemap& operator=(Cubemap&& o) noexcept
	{
		assert(this != &o);
		std::swap(m_vbo, o.m_vbo);
		std::swap(m_vao, o.m_vao);
		std::swap(m_cubemap, o.m_cubemap);
		std::swap(m_envmap, o.m_envmap);
		return *this;
	}

	void load_textures(const std::initializer_list<std::string_view>& textures, std::string_view basedir);

	void draw(uint32_t shader, const glm::mat4& view, const glm::mat4& projection) noexcept;
};
