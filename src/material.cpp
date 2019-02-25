#include <rendercat/material.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/uniform.hpp>
#include <string>
#include <fmt/core.h>
#include <stb_image.h>
#include <zcm/vec2.hpp>


using namespace rc;

template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
static inline constexpr auto repr(T e)
{
	return static_cast<std::underlying_type_t<T>>(e);
}

static inline constexpr uint32_t clear_mask(uint32_t val, uint32_t mask)
{
	return val & ~mask;
}

static inline constexpr bool test(uint32_t flags, Texture::Kind kind)
{
	return (flags & repr(kind)) == repr(kind);
}


static ImageTexture2D  _default_diffuse;


Material::Material(const std::string_view name_) : name(name_) { }

void Material::set_default_diffuse(const std::string_view path) noexcept
{
	//stbi_set_flip_vertically_on_load(1);
	_default_diffuse = ImageTexture2D::fromFile(path, Texture::ColorSpace::sRGB);

	assert(_default_diffuse.valid() && "invalid default diffuse");

	_default_diffuse.set_filtering(Texture::MinFilter::Nearest, Texture::MagFilter::Nearest);
	_default_diffuse.set_anisotropy(1);
}

Material Material::create_default_material()
{
	assert(_default_diffuse.valid() && "default diffuse not yet set or invalid");
	Material missing{"missing"};
	missing.base_color_map = _default_diffuse.share();
	missing.set_texture_kind(Texture::Kind::BaseColor, true);
	return missing;
}


bool Material::valid() const
{
	using namespace Texture;
	bool ret = true;
	if(alpha_mode == AlphaMode::Unknown) {
		fmt::print(stderr, "[material] invalid material: has alpha channel but AlphaMode is Unknown\n");
		ret = false;
	}
	if(has_texture_kind(Kind::SpecularGlossiness)) {
		if(has_texture_kind(Kind::RoughnessMetallic) || has_texture_kind(Kind::Occlusion)) {
			fmt::print(stderr, "[material] invalid material: Has PBR and Phong texture maps!\n");
			ret = false;
		}
	}
	return ret;
}


void Material::bind(uint32_t s) const noexcept
{
	using namespace Texture;

	unif::v4(s, "material.diffuse",            base_color_factor);
	unif::v4(s, "material.specular",           zcm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
	unif::v3(s, "material.emission",           emissive_factor);
	unif::f1(s, "material.normal_scale",       normal_scale);
	unif::f1(s, "material.roughness",          roughness_factor);
	unif::f1(s, "material.metallic",           metallic_factor);
	unif::f1(s, "material.occlusion_strength", occlusion_strength);
	unif::f1(s, "material.alpha_cutoff",       alpha_cutoff);

	auto flags = m_flags;
	if(alpha_mode == Texture::AlphaMode::Mask) {
		flags |= RC_SHADER_TEXTURE_ALPHA_MASK;
	}
	if(alpha_mode == Texture::AlphaMode::Blend) {
		flags |= RC_SHADER_TEXTURE_BLEND;
	}

	if(has_texture_kind(Kind::BaseColor)) {
		if(!base_color_map.bind_to_unit(RC_FRAGMENT_SHADER_TEXTURE_BINDING_DIFFUSE)) {
			_default_diffuse.bind_to_unit(RC_FRAGMENT_SHADER_TEXTURE_BINDING_DIFFUSE);
		}
	}
	if(has_texture_kind(Kind::Normal)) {
		normal_map.bind_to_unit(RC_FRAGMENT_SHADER_TEXTURE_BINDING_NORMAL);
	}

	if (has_texture_kind(Kind::OcclusionSeparate)) {
		occlusion_map.bind_to_unit(RC_FRAGMENT_SHADER_TEXTURE_BINDING_OCCLUSION);
	} else if (has_texture_kind(Kind::Occlusion)) {
		occlusion_roughness_metallic_map.bind_to_unit(RC_FRAGMENT_SHADER_TEXTURE_BINDING_OCCLUSION);
	}

	if (has_texture_kind(Kind::RoughnessMetallic)) {
		occlusion_roughness_metallic_map.bind_to_unit(RC_FRAGMENT_SHADER_TEXTURE_BINDING_ROUGHNESS_METALLIC);
	}

	if (has_texture_kind(Kind::Emission)) {
		emission_map.bind_to_unit(RC_FRAGMENT_SHADER_TEXTURE_BINDING_EMISSION);
	}

	unif::i1(s,  "material.type", flags);
}

void Material::set_base_color_map(ImageTexture2D&& map)
{
	base_color_map = std::move(map);
	set_texture_kind(Texture::Kind::BaseColor, true);
}

void Material::set_normal_map(ImageTexture2D&& map)
{
	normal_map = std::move(map);
	set_texture_kind(Texture::Kind::Normal, true);
}


void Material::set_emission_map(ImageTexture2D&& map)
{
	emission_map = std::move(map);
	set_texture_kind(Texture::Kind::Emission, true);
}

void Material::set_occlusion_map(ImageTexture2D && map)
{
	occlusion_map = std::move(map);
	set_texture_kind(Texture::Kind::OcclusionSeparate, true);
}

bool Material::has_texture_kind(Texture::Kind k) const noexcept
{
	return test(m_flags, k);
}

bool Material::set_texture_kind(Texture::Kind k, bool has_map) noexcept
{
	using namespace Texture;
	bool prev = has_texture_kind(k);
	if (has_map) {
		m_flags |= repr(k);
	} else {
		auto prev_flags = m_flags;
		m_flags = clear_mask(m_flags, repr(k));
		cleanup(prev_flags, m_flags);
	}

	return prev;
}

void Material::cleanup(uint32_t prev_flags, uint32_t new_flags) noexcept
{
	using namespace Texture;

	auto had_texture_kind = [](auto prev_flags, auto new_flags, auto kind)
	{
		return test(prev_flags, kind) && !test(new_flags, kind);
	};

	if (had_texture_kind(prev_flags, new_flags, Kind::BaseColor)) {
		base_color_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::Normal)) {
		normal_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::Emission)) {
		emission_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::SpecularGlossiness)) {
		specular_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::OcclusionSeparate)) {
		occlusion_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::OcclusionRoughnessMetallic)) {
		occlusion_roughness_metallic_map.reset();
	}
}

static ImageTexture2D try_from_cache_or_load(std::string&& path, Texture::ColorSpace space)
{
	ImageTexture2D ret;
	auto cached = Texture::Cache::get(path);
	if(cached) {
		ret = cached->share();
	} else {
		ret = ImageTexture2D::fromFile(path, space);
		if(ret.valid()) {
			Texture::Cache::add(std::move(path), ret.share());
		}
	}
	return ret;
}

ImageTexture2D Material::load_image_texture(std::string_view basedir, std::string_view name, Texture::ColorSpace colorspace)
{
	std::string filepath;
	filepath = basedir;
	if(basedir.find_last_of('/') != basedir.length()-1)
		filepath.push_back('/');
	filepath.append(name);

	return try_from_cache_or_load(std::move(filepath), colorspace);
}
