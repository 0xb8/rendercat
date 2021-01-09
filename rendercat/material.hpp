#pragma once
#include <rendercat/common.hpp>
#include <rendercat/texture2d.hpp>
#include <rendercat/uniform.hpp>
#include <zcm/vec4.hpp>
#include <zcm/vec3.hpp>
#include <string>

namespace rc {

class TextureCache;
struct Material
{
	explicit Material(const std::string_view name_);
	~Material();
	RC_DISABLE_COPY(Material)
	Material(Material&&) noexcept;
	Material& operator=(Material&&) noexcept;

	static void set_default_diffuse(const std::string_view path) noexcept;
	static void delete_default_diffuse() noexcept;
	static Material create_default_material();
	static ImageTexture2D load_image_texture(std::string_view basedir, std::string_view path, Texture::ColorSpace colorspace);

	bool valid() const;
	void flush();
	void map(bool init=false);
	void unmap();
	void bind(uint32_t s) const noexcept;

	void set_base_color_factor(zcm::vec4) noexcept;
	void set_emissive_factor(zcm::vec3) noexcept;
	void set_normal_scale(float) noexcept;
	void set_roughness_factor(float) noexcept;
	void set_metallic_factor(float) noexcept;
	void set_occlusion_strength(float) noexcept;
	void set_alpha_cutoff(float) noexcept;
	void set_alpha_mode(Texture::AlphaMode) noexcept;
	void set_double_sided(bool) noexcept;

	void set_base_color_map(ImageTexture2D&&);
	void set_normal_map(ImageTexture2D&&);
	void set_emission_map(ImageTexture2D&&);
	void set_occlusion_map(ImageTexture2D&&);

	bool has_texture_kind(Texture::Kind) const noexcept;
	bool set_texture_kind(Texture::Kind, bool has_map) noexcept;

	Texture::AlphaMode alpha_mode() const noexcept;
	bool double_sided() const noexcept;

	struct UniformData {
		zcm::vec4          base_color_factor  = 1.0f;
		zcm::vec3          emissive_factor    = 0.0f;
		float              normal_scale       = 1.0f;
		float              roughness_factor   = 1.0f;
		float              metallic_factor    = 1.0f;
		float              occlusion_strength = 1.0f;
		float              alpha_cutoff = 0.5f;
		int                type = 0;
	};

	struct Textures {
		ImageTexture2D     base_color_map;
		ImageTexture2D     normal_map;
		ImageTexture2D     occlusion_roughness_metallic_map;
		ImageTexture2D     occlusion_map;
		ImageTexture2D     emission_map;
	};
	Textures           textures;
	std::string        name;


private:
	friend struct Scene;
	void cleanup(uint32_t prev_flags, uint32_t new_flags) noexcept;
	void set_shader_flags();

	unif::buf<UniformData> m_unif_data;
	uint32_t           m_flags = 0;
	Texture::AlphaMode m_alpha_mode = Texture::AlphaMode::Opaque;
	bool               m_double_sided = true;

public:

	UniformData *data = nullptr; // pointer to mapped params UBO




};

} // namespace rc
