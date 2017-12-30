#include <rendercat/material.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/uniform.hpp>
#include <fmt/format.h>
#include <stb_image.h>


template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
static inline constexpr auto repr(T e)
{
	return static_cast<std::underlying_type_t<T>>(e);
}

static inline constexpr uint32_t clear_mask(uint32_t val, uint32_t mask)
{
	return val & ~mask;
}

using namespace rc;

ImageTexture2D  Material::default_diffuse;
TextureCache*   Material::cache = nullptr;

void Material::set_texture_cache(TextureCache * c)
{
	assert(c && "invalid texture cache pointer");
	cache = c;
}

void Material::set_default_diffuse(const std::string_view path) noexcept
{
	stbi_set_flip_vertically_on_load(1);
	default_diffuse = ImageTexture2D::fromFile(path, Texture::ColorSpace::sRGB);

	assert(default_diffuse.valid() && "invalid default diffuse");

	default_diffuse.setFiltering(Texture::MipMapping::Disable, Texture::Filtering::Nearest);
	default_diffuse.setAnisotropy(1);
}

Material Material::create_default_material()
{
	assert(default_diffuse.valid() && "default diffuse not yet set or invalid");
	Material missing{"missing"};
	missing.m_diffuse_map = default_diffuse.share();
	return missing;
}

Material::Material(const std::string & name_) : name(name_)
{

}

static bool test(uint32_t f, Texture::Kind t) noexcept
{
	return f & repr(t);
}

bool Material::valid() const
{
	using namespace Texture;
	bool ret = true;
	if(m_alpha_mode == AlphaMode::Unknown) {
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

void Material::setAlphaMode(Texture::AlphaMode m, float alpha_cutoff) noexcept
{
	m_alpha_mode = m;
	m_alpha_cutoff = alpha_cutoff;
}

void Material::bind(uint32_t s) const noexcept
{
	using namespace Texture;

	unif::v4(s, "material.diffuse",   m_diffuse_color);
	unif::v4(s, "material.specular",  glm::vec4(m_specular_color, m_shininess));
	unif::f1(s, "material.alpha_cutoff", m_alpha_cutoff);

	auto flags = m_flags;
	if(m_alpha_mode == Texture::AlphaMode::Mask) {
		flags |= RC_SHADER_TEXTURE_ALPHA_MASK;
	}
	if(m_alpha_mode == Texture::AlphaMode::Blend) {
		flags |= RC_SHADER_TEXTURE_BLEND;
	}
	unif::i1(s,  "material.type", flags);

	if(test(m_flags, Kind::Diffuse)) {
		if(!m_diffuse_map.bindToUnit(0)) {
			default_diffuse.bindToUnit(0);
		}
	}
	if(test(m_flags, Kind::Normal)) {
		m_normal_map.bindToUnit(1);
	}
	if(test(m_flags, Kind::Specular)) {
		m_specular_map.bindToUnit(2);
	}
}

bool Material::hasTextureKind(Texture::Kind k) const noexcept
{
	return m_flags & repr(k);
}

static ImageTexture2D try_from_cache_or_load(TextureCache* cache, std::string&& path, Texture::ColorSpace space)
{
	assert(cache != nullptr);
	ImageTexture2D *cached = nullptr;
	ImageTexture2D ret;
	cached = cache->get(path);
	if(cached) {
		ret = cached->share();
	} else {
		ret = ImageTexture2D::fromFile(path, space);
		if(ret.valid()) {
			cache->add(std::move(path), ret.share());
		}
	}
	return ret;
}


void Material::addDiffuseMap(const std::string_view name, const std::string_view basedir)
{
	std::string diffuse_path;
	diffuse_path = basedir;
	if(basedir.find_last_of('/') != basedir.length()-1)
		diffuse_path.push_back('/');
	diffuse_path.append(name);

	m_diffuse_map = try_from_cache_or_load(cache, std::move(diffuse_path), Texture::ColorSpace::sRGB);


	if(m_diffuse_map.valid())
	{
		m_flags |= repr(Texture::Kind::Diffuse);
		if(m_diffuse_map.channels() == 4) {
			m_alpha_mode = Texture::AlphaMode::Unknown;
		} else {
			m_alpha_mode = Texture::AlphaMode::Opaque;
		}
	}
}

void Material::addNormalMap(const std::string_view name, const std::string_view basedir)
{
	std::string normal_path;
	normal_path = basedir;
	if(basedir.find_last_of('/') != basedir.length()-1)
		normal_path.push_back('/');
	normal_path.append(name);

	m_normal_map = try_from_cache_or_load(cache, std::move(normal_path), Texture::ColorSpace::Linear);

	if(m_normal_map.valid()) {
		m_flags |= repr(Texture::Kind::Normal);
	}
}

void Material::addSpecularMap(const std::string_view name, const std::string_view basedir)
{
	std::string specular_path;
	specular_path = basedir;
	if(basedir.find_last_of('/') != basedir.length()-1)
		specular_path.push_back('/');
	specular_path.append(name);

	m_specular_map = try_from_cache_or_load(cache, std::move(specular_path), Texture::ColorSpace::Linear);

	if(m_specular_map.valid()) {
		m_flags |= repr(Texture::Kind::Specular);
	}
}

