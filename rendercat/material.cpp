#include <rendercat/material.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/uniform.hpp>
#include <string>
#include <fmt/core.h>
#include <stb_image.h>
#include <zcm/vec2.hpp>
#include <cassert>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

#include <tracy/Tracy.hpp>
using namespace gl45core;
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


Material::Material(const std::string_view name_) : name(name_) {
	m_unif_data.set_label(fmt::format("material: {}", name));
}

Material::~Material()
{
	unmap();
}


#define IMPLEMENT_MATERIAL_MOVE(x, ret) x {                                    \
	name = std::move(o.name);                                              \
	textures = std::move(o.textures);                                      \
	m_unif_data = std::move(o.m_unif_data);                                \
	m_double_sided = o.m_double_sided;                                     \
	m_alpha_mode = o.m_alpha_mode;                                         \
	m_flags = o.m_flags;                                                   \
	ret;                                                                   \
}

IMPLEMENT_MATERIAL_MOVE(Material::Material(Material && o) noexcept, )
IMPLEMENT_MATERIAL_MOVE(Material& Material::operator=(Material && o) noexcept, return *this)
#undef IMPLEMENT_MATERIAL_MOVE


void Material::set_default_diffuse(const std::string_view path) noexcept
{
	//stbi_set_flip_vertically_on_load(1);
	_default_diffuse = ImageTexture2D::fromFile(path, Texture::ColorSpace::sRGB);

	assert(_default_diffuse.valid() && "invalid default diffuse");

	_default_diffuse.set_filtering(Texture::MinFilter::Nearest, Texture::MagFilter::Nearest);
	_default_diffuse.set_anisotropy(1);
}

void Material::delete_default_diffuse() noexcept
{
	_default_diffuse.reset();
}

Material Material::create_default_material()
{
	assert(_default_diffuse.valid() && "default diffuse not yet set or invalid");
	Material missing{"missing"};
	missing.textures.base_color_map = _default_diffuse.share();
	missing.set_texture_kind(Texture::Kind::BaseColor, true);
	return missing;
}


bool Material::valid() const
{
	using namespace Texture;
	bool ret = true;
	if(m_alpha_mode == AlphaMode::Unknown) {
		fmt::print(stderr, "[material] invalid material: has alpha channel but AlphaMode is Unknown\n");
		ret = false;
	}
	return ret;
}

void Material::flush()
{
	m_unif_data.flush();
}

void Material::map()
{
	m_unif_data.map(false);
}

void Material::unmap()
{
	m_unif_data.unmap();
}


void Material::bind(uint32_t) const noexcept
{
	using namespace Texture;
	assert(m_unif_data);
	m_unif_data.bind(0);

	if(has_texture_kind(Kind::BaseColor)) {
		if(!bind_to_unit(textures.base_color_map, RC_FRAGMENT_SHADER_TEXTURE_BINDING_DIFFUSE)) {
			bind_to_unit(_default_diffuse, RC_FRAGMENT_SHADER_TEXTURE_BINDING_DIFFUSE);
		}
	}
	if(has_texture_kind(Kind::Normal)) {
		bind_to_unit(textures.normal_map, RC_FRAGMENT_SHADER_TEXTURE_BINDING_NORMAL);
	}

	if (has_texture_kind(Kind::OcclusionSeparate)) {
		bind_to_unit(textures.occlusion_map, RC_FRAGMENT_SHADER_TEXTURE_BINDING_OCCLUSION);
	} else if (has_texture_kind(Kind::Occlusion)) {
		bind_to_unit(textures.occlusion_roughness_metallic_map, RC_FRAGMENT_SHADER_TEXTURE_BINDING_OCCLUSION);
	}

	if (has_texture_kind(Kind::RoughnessMetallic)) {
		bind_to_unit(textures.occlusion_roughness_metallic_map, RC_FRAGMENT_SHADER_TEXTURE_BINDING_ROUGHNESS_METALLIC);
	}

	if (has_texture_kind(Kind::Emission)) {
		bind_to_unit(textures.emission_map, RC_FRAGMENT_SHADER_TEXTURE_BINDING_EMISSION);
	}
}

Texture::AlphaMode Material::alpha_mode() const noexcept
{
	return m_alpha_mode;
}

bool Material::double_sided() const noexcept
{
	return m_double_sided;
}


void Material::set_base_color_factor(zcm::vec4 f) noexcept
{
	assert(data());
	data()->base_color_factor = f;
}

void Material::set_emissive_factor(zcm::vec3 f) noexcept
{
	assert(data());
	data()->emissive_factor = f;
}

void Material::set_normal_scale(float f) noexcept
{
	assert(data());
	data()->normal_scale = f;
}

void Material::set_roughness_factor(float f) noexcept
{
	assert(data());
	data()->roughness_factor = f;
}

void Material::set_metallic_factor(float f) noexcept
{
	assert(data());
	data()->metallic_factor = f;
}

void Material::set_occlusion_strength(float s) noexcept
{
	assert(data());
	data()->occlusion_strength = s;
}

void Material::set_alpha_cutoff(float c) noexcept
{
	assert(data());
	data()->alpha_cutoff = c;
}

void Material::set_alpha_mode(Texture::AlphaMode m) noexcept
{
	assert(data());
	m_alpha_mode = m;
	set_shader_flags();
}

void Material::set_double_sided(bool d) noexcept
{
	assert(data());
	m_double_sided = d;
}

void Material::set_base_color_map(ImageTexture2D&& map)
{
	textures.base_color_map = std::move(map);
	set_texture_kind(Texture::Kind::BaseColor, true);
}

void Material::set_normal_map(ImageTexture2D&& map)
{
	textures.normal_map = std::move(map);
	set_texture_kind(Texture::Kind::Normal, true);
}


void Material::set_emission_map(ImageTexture2D&& map)
{
	textures.emission_map = std::move(map);
	set_texture_kind(Texture::Kind::Emission, true);
}

void Material::set_occlusion_map(ImageTexture2D && map)
{
	textures.occlusion_map = std::move(map);
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
	set_shader_flags();

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
		textures.base_color_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::Normal)) {
		textures.normal_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::Emission)) {
		textures.emission_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::OcclusionSeparate)) {
		textures.occlusion_map.reset();
	}
	if (had_texture_kind(prev_flags, new_flags, Kind::OcclusionRoughnessMetallic)) {
		textures.occlusion_roughness_metallic_map.reset();
	}
}

void Material::set_shader_flags()
{
	assert(data());
	auto flags = m_flags;
	if(m_alpha_mode == Texture::AlphaMode::Mask) {
		flags |= RC_SHADER_TEXTURE_ALPHA_MASK;
	}
	if(m_alpha_mode == Texture::AlphaMode::Blend) {
		flags |= RC_SHADER_TEXTURE_BLEND;
	}
	if (has_texture_kind(Texture::Kind::Normal)) {
		if (textures.normal_map.channels() == 2) {
			flags |= RC_SHADER_TEXTURE_NORMAL_WITHOUT_Z;
		} else if (textures.normal_map.channels() == 3) {
			flags = clear_mask(flags, RC_SHADER_TEXTURE_NORMAL_WITHOUT_Z);
		}
	}
	data()->type = flags;
}

Material::UniformData * Material::data() const
{
	return m_unif_data.data();
}

static ImageTexture2D try_from_cache_or_load(std::string&& path, Texture::ColorSpace space)
{
	ZoneScoped;
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
