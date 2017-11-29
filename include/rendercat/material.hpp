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

	Material(const std::string& name_);
	~Material() = default;

	RC_DEFAULT_MOVE(Material)

	bool valid() const;

	bool hasTextureKind(Texture::Kind) const noexcept;

	void bind(uint32_t s) const noexcept;

	void diffuseColor(const glm::vec3& color) noexcept
	{
		m_diffuse_color = color;
		m_flags |= 1 << 6;
	}
	void specularColorShininess(const glm::vec3& color, float shininess = 8.0f) noexcept
	{
		m_specular_color = color;
		m_shininess = shininess;
	}
	void rougnessMetallic(float rougness, float metallic) noexcept
	{
		m_roughness = rougness;
		m_metallic = metallic;
	}
	void emissiveColor(const glm::vec3& color) noexcept
	{
		m_emissive_color = color;
	}

	void addDiffuseMap(const std::string_view name, const std::string_view basedir);
	void addNormalMap(const std::string_view name, const std::string_view basedir);
	void addSpecularMap(const std::string_view name, const std::string_view basedir);

	std::string name;
	Texture::AlphaMode alpha_mode = Texture::AlphaMode::Unknown;
	bool face_culling_enabled = true;

private:
	RC_DISABLE_COPY(Material)
	static TextureCache* cache;
	static ImageTexture2D default_diffuse;

	uint32_t  m_flags = 0;

	glm::vec3 m_diffuse_color;
	glm::vec3 m_specular_color;
	glm::vec3 m_emissive_color;
	float     m_shininess = 0.0f;
	float     m_roughness = 1.0f;
	float     m_metallic  = 0.0f;

	ImageTexture2D  m_diffuse_map;
	ImageTexture2D  m_normal_map;
	ImageTexture2D  m_specular_map;
};

} // namespace rc
