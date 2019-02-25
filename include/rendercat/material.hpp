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
	friend struct Scene;
	void cleanup(uint32_t prev_flags, uint32_t new_flags) noexcept;
public:

	explicit Material(const std::string_view name_);
	~Material() = default;

	RC_DEFAULT_MOVE(Material)
	RC_DISABLE_COPY(Material)

	ImageTexture2D     base_color_map;
	ImageTexture2D     normal_map;
	ImageTexture2D     specular_map;
	ImageTexture2D     occlusion_roughness_metallic_map;
	ImageTexture2D     occlusion_map;
	ImageTexture2D     emission_map;

	std::string        name;
	zcm::vec4          base_color_factor  = 1.0f;
	zcm::vec3          emissive_factor    = 0.0f;
	float              normal_scale       = 1.0f;
	float              metallic_factor    = 1.0f;
	float              roughness_factor   = 1.0f;
	float              occlusion_strength = 1.0f;
	float              alpha_cutoff = 0.5f;
	bool               double_sided = true;
	Texture::AlphaMode alpha_mode = Texture::AlphaMode::Opaque;

	static void set_default_diffuse(const std::string_view path) noexcept;
	static Material create_default_material();
	static ImageTexture2D load_image_texture(std::string_view basedir, std::string_view path, Texture::ColorSpace colorspace);

	bool valid() const;
	void bind(uint32_t s) const noexcept;

	void set_base_color_map(ImageTexture2D&&);
	void set_normal_map(ImageTexture2D&&);
	void set_emission_map(ImageTexture2D&&);
	void set_occlusion_map(ImageTexture2D&&);

	bool has_texture_kind(Texture::Kind) const noexcept;
	bool set_texture_kind(Texture::Kind, bool has_map) noexcept;
};

} // namespace rc
