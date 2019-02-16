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

static bool test(uint32_t f, Texture::Kind t) noexcept
{
	return f & repr(t);
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
	if(test(m_flags, Kind::Specular)) {
		if(test(m_flags, Kind::Roughness) || test(m_flags, Kind::Metallic) || test(m_flags, Kind::Occlusion)) {
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
	unif::v2(s, "material.roughness_metallic", zcm::vec2(roughness_factor, metallic_factor));
	unif::f1(s, "material.alpha_cutoff", alpha_cutoff);

	auto flags = m_flags;
	if(alpha_mode == Texture::AlphaMode::Mask) {
		flags |= RC_SHADER_TEXTURE_ALPHA_MASK;
	}
	if(alpha_mode == Texture::AlphaMode::Blend) {
		flags |= RC_SHADER_TEXTURE_BLEND;
	}

	if(test(m_flags, Kind::Diffuse)) {
		if(!diffuse_map.bind_to_unit(0)) {
			_default_diffuse.bind_to_unit(0);
		}
	}
	if(test(m_flags, Kind::Normal)) {
		normal_map.bind_to_unit(1);
	}
	if(test(m_flags, Kind::Specular)) {
		specular_map.bind_to_unit(2);
	}

	if (test(m_flags, Kind::Occlusion)) {
		flags |= RC_SHADER_TEXTURE_KIND_OCCLUSION;
		occlusion_map.bind_to_unit(4);
	}
	if (test(m_flags, Kind::MetallicRoughness)) {
		flags |= RC_SHADER_TEXTURE_KIND_ROUGNESS | RC_SHADER_TEXTURE_KIND_METALLIC;
		metallic_roughness_map.bind_to_unit(3);
	}
	unif::i1(s,  "material.type", flags);
}

bool Material::has_texture_kind(Texture::Kind k) const noexcept
{
	return test(m_flags, k);
}

bool Material::set_texture_kind(Texture::Kind k, bool has) noexcept
{
	bool prev = test(m_flags, k);
	if (has) {
		m_flags |= repr(k);
	} else {
		m_flags = clear_mask(m_flags, repr(k));
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
