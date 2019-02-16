#pragma once
#include <rendercat/common.hpp>
#include <rendercat/texture2d.hpp>
#include <zcm/vec4.hpp>
#include <zcm/vec3.hpp>
#include <string>

namespace rc {

class TextureCache;
class Material
{
	uint32_t  m_flags = 0;
	friend class Scene;
public:

	explicit Material(const std::string_view name_);
	~Material() = default;

	RC_DEFAULT_MOVE(Material)
	RC_DISABLE_COPY(Material)

	ImageTexture2D     diffuse_map;
	ImageTexture2D     normal_map;
	ImageTexture2D     specular_map;
	ImageTexture2D     metallic_roughness_map;
	ImageTexture2D     occlusion_map;
	ImageTexture2D     emission_map;

	std::string        name;
	zcm::vec4          diffuse_factor  = 1.0f;
	zcm::vec3          specular_factor = 1.0f;
	zcm::vec3          emissive_factor = 1.0f;
	float              metallic_factor     = 1.0f;
	float              roughness_factor    = 1.0f;

	float              alpha_cutoff = 0.5f;
	float              shininess    = 8.0f;
	bool               double_sided = true;
	Texture::AlphaMode alpha_mode;

	static void set_default_diffuse(const std::string_view path) noexcept;
	static Material create_default_material();
	static ImageTexture2D load_image_texture(std::string_view basedir, std::string_view path, Texture::ColorSpace colorspace);

	bool valid() const;
	void bind(uint32_t s) const noexcept;
	bool has_texture_kind(Texture::Kind) const noexcept;
	bool set_texture_kind(Texture::Kind, bool) noexcept;
};

} // namespace rc
