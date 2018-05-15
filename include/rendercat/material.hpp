#pragma once
#include <rendercat/common.hpp>
#include <rendercat/texture2d.hpp>

namespace rc {

class TextureCache;
struct Material
{
	friend class Scene;
	static void set_texture_cache(TextureCache* c);
	static void set_default_diffuse(const std::string_view path) noexcept;
	static Material create_default_material();

	explicit Material(const std::string_view name_) : name(name_) { }
	~Material() = default;

	RC_DEFAULT_MOVE(Material)
	RC_DISABLE_COPY(Material)

	bool valid() const;
	bool has_texture_kind(Texture::Kind) const noexcept;

	void bind(uint32_t s) const noexcept;

	void set_diffuse_color(const glm::vec4& color) noexcept
	{
		m_diffuse_color = color;
	}

	void set_specular_color_shininess(const glm::vec3& color, float shininess = 8.0f) noexcept
	{
		m_specular_color = color;
		m_shininess = shininess;
	}

	void set_rougness_metallic(float rougness, float metallic) noexcept
	{
		m_roughness = rougness;
		m_metallic = metallic;
	}

	void set_emissive_color(const glm::vec3& color) noexcept
	{
		m_emissive_color = color;
	}

	void add_diffuse_map(const std::string_view name, const std::string_view basedir);
	void add_normal_map(const std::string_view name, const std::string_view basedir);
	void add_specular_map(const std::string_view name, const std::string_view basedir);

	Texture::AlphaMode alpha_mode() const noexcept
	{
		return m_alpha_mode;
	}
	void set_alpha_mode(Texture::AlphaMode m, float alpha_cutoff = 0.5f) noexcept;

	std::string name;
	bool face_culling_enabled = true;

private:

	static TextureCache* cache;
	static ImageTexture2D default_diffuse;

	uint32_t  m_flags = 0;

	glm::vec4 m_diffuse_color;
	glm::vec3 m_specular_color;
	glm::vec3 m_emissive_color;
	float     m_alpha_cutoff = 0.5f;
	float     m_shininess = 0.0f;
	float     m_roughness = 1.0f;
	float     m_metallic  = 0.0f;


	ImageTexture2D  m_diffuse_map;
	ImageTexture2D  m_normal_map;
	ImageTexture2D  m_specular_map;

	Texture::AlphaMode m_alpha_mode = Texture::AlphaMode::Unknown;
};

} // namespace rc
