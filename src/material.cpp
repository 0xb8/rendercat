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
	missing.diffuse_map = _default_diffuse.share();
	missing.set_texture_kind(Texture::Kind::Diffuse, true);
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
	if(has_texture_kind(Kind::Specular)) {
		if(has_texture_kind(Kind::Roughness) || has_texture_kind(Kind::Metallic) || has_texture_kind(Kind::Occlusion)) {
			fmt::print(stderr, "[material] invalid material: Has PBR and Phong texture maps!\n");
			ret = false;
		}
	}
	return ret;
}


void Material::bind(uint32_t s) const noexcept
{
	using namespace Texture;

	unif::v4(s, "material.diffuse",   diffuse_factor);
	unif::v4(s, "material.specular",  zcm::vec4(specular_factor, shininess));
	unif::v3(s, "material.emission",  emissive_factor);
	unif::f1(s, "material.normal_scale", normal_scale);
	unif::f1(s, "material.roughness", roughness_factor);
	unif::f1(s, "material.metallic",  metallic_factor);
	unif::f1(s, "material.occlusion_strength", occlusion_strength);
	unif::f1(s, "material.alpha_cutoff", alpha_cutoff);

	auto flags = m_flags;
	if(alpha_mode == Texture::AlphaMode::Mask) {
		flags |= RC_SHADER_TEXTURE_ALPHA_MASK;
	}
	if(alpha_mode == Texture::AlphaMode::Blend) {
		flags |= RC_SHADER_TEXTURE_BLEND;
	}

	if(has_texture_kind(Kind::Diffuse)) {
		if(!diffuse_map.bind_to_unit(0)) {
			_default_diffuse.bind_to_unit(0);
		}
	}
	if(has_texture_kind(Kind::Normal)) {
		normal_map.bind_to_unit(1);
	}
	if(has_texture_kind(Kind::Specular)) {
		specular_map.bind_to_unit(2);
	}
	if (has_texture_kind(Kind::Roughness) || has_texture_kind(Kind::Metallic) || has_texture_kind(Kind::Occlusion)) {
		flags |= RC_SHADER_TEXTURE_KIND_ORM;
		occluion_roughness_metallic_map.bind_to_unit(3);
	}
	if (has_texture_kind(Kind::Emission)) {
		flags |= RC_SHADER_TEXTURE_KIND_EMISSION;
		emission_map.bind_to_unit(4);
	}

	unif::i1(s,  "material.type", flags);
}

bool Material::has_texture_kind(Texture::Kind k) const noexcept
{
	return m_flags & repr(k);
}

bool Material::set_texture_kind(Texture::Kind k, bool has) noexcept
{
	bool prev = has_texture_kind( k);
	if (has) {
		m_flags |= repr(k);
	} else {
		m_flags = clear_mask(m_flags, repr(k));
	}

	using namespace Texture;

	bool has_rougness = has_texture_kind(Kind::Roughness);
	bool has_metallic = has_texture_kind(Kind::Metallic);
	bool has_ocllusion = has_texture_kind(Kind::Occlusion);

	if (has_metallic || has_rougness || has_ocllusion) {
		auto orm_mask = SwizzleMask{};
		orm_mask.red   = has_ocllusion ? ChannelValue::Red   : ChannelValue::One;
		orm_mask.green = has_rougness  ? ChannelValue::Green : ChannelValue::One;
		orm_mask.blue  = has_metallic  ? ChannelValue::Blue  : ChannelValue::One;
		occluion_roughness_metallic_map.set_swizzle_mask(orm_mask);
	}

	return prev;
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
