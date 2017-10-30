#include <rendercat/material.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/uniform.hpp>
#include <stb_image.h>
#include <cmath>
#include <utility>
#include <fmt/format.h>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding/gl45ext/enum.h>

using namespace gl45core;
static constexpr auto GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT = gl45ext::GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT;
static constexpr auto GL_TEXTURE_MAX_ANISOTROPY_EXT = gl45ext::GL_TEXTURE_MAX_ANISOTROPY_EXT;
static float max_anisotropic_filtering_samples = 2.0f; // required by spec

GLuint        Material::default_diffuse = 0;
TextureCache* Material::cache = nullptr;

static GLuint load_texture(const std::string& path, bool linear, int desired_channels = 0, int* num_channels = nullptr)
{
	static const int   max_levels = 14;
	if(max_anisotropic_filtering_samples == 2.0f) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropic_filtering_samples);
		GLint max_tex_size{}, max_buffer_size;
		glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &max_tex_size);
		glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_buffer_size);
		fmt::print("[material] limits:\n"
		           "    AF samples:   {}\n"
		           "    Texture size: {}\n"
		           "    Texture buf:  {}\n",
		           max_anisotropic_filtering_samples,
		           max_tex_size,
		           max_buffer_size);
	}

	GLuint tex = 0;
	int width, height, nrChannels;
	auto data = stbi_load(path.data(), &width, &height, &nrChannels, desired_channels);
	if (data) {
		int nr_levels = std::min((int)std::round(std::log2(std::min(width, height))), max_levels);

		glCreateTextures(GL_TEXTURE_2D, 1, &tex);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_S,     GL_REPEAT);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_T,     GL_REPEAT);
		glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropic_filtering_samples);

		if(desired_channels == 0) desired_channels = 1000;

		switch (std::min(nrChannels, desired_channels)) {
		case 1:
			static const GLenum swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
			glTextureParameteriv(tex, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			glTextureStorage2D(tex, nr_levels, GL_R8, width, height); // always linear
			glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data);
			break;
		case 3:
			glTextureStorage2D(tex, nr_levels, (linear ? GL_RGB8 : GL_SRGB8), width, height);
			glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
			break;
		case 4:
			glTextureStorage2D(tex, nr_levels, (linear ? GL_RGBA8 : GL_SRGB8_ALPHA8), width, height);
			glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			break;
		default:
			fmt::print(stderr, "[material] invalid channel count: {} in [{}]\n", nrChannels, path);
			assert(false);
			break;
		}

		glGenerateTextureMipmap(tex);
		stbi_image_free(data);

		if(num_channels)
			*num_channels = nrChannels;
	}
	return tex;
}

void Material::set_texture_cache(TextureCache * c)
{
	assert(c && "invalid texture cache pointer");
	cache = c;
}

void Material::set_default_diffuse() noexcept
{
	stbi_set_flip_vertically_on_load(1);
	auto diffuse = load_texture("assets/materials/missing.png", true);

	assert(diffuse && "invalid default diffuse");
	default_diffuse = diffuse;
}

Material::~Material() {
	glDeleteTextures(1, &m_diffuse_map);
	glDeleteTextures(1, &m_normal_map);
	glDeleteTextures(1, &m_specular_map);
}

Material &Material::operator =(Material&& o) noexcept
{
	assert(this != &o);
	m_specular_color = o.m_specular_color;
	m_shininess      = o.m_shininess;
	flags            = o.flags;

	name = std::move(o.name);
	glDeleteTextures(1, &m_diffuse_map);
	glDeleteTextures(1, &m_normal_map);
	glDeleteTextures(1, &m_specular_map);
	m_diffuse_map = std::exchange(o.m_diffuse_map, 0);
	m_normal_map = std::exchange(o.m_normal_map, 0);
	m_specular_map = std::exchange(o.m_specular_map, 0);

	return *this;
}

Material::Material(Material&& o) noexcept :
	name(std::move(o.name)),
	flags(o.flags),
	m_specular_color(o.m_specular_color),
	m_shininess(o.m_shininess),
	m_diffuse_map(std::exchange(o.m_diffuse_map, 0)),
	m_normal_map(std::exchange(o.m_normal_map, 0)),
	m_specular_map(std::exchange(o.m_specular_map, 0)) { }

void Material::bind(GLuint s) const noexcept
{
	unif::v3(s,  "material.specular",  m_specular_color);
	unif::f1(s,  "material.shininess", m_shininess);
	unif::i1(s,  "material.type",      flags);

	glBindTextureUnit(0, m_diffuse_map);
	glBindTextureUnit(1, m_normal_map);
	glBindTextureUnit(2, m_specular_map);
}

void Material::addDiffuseMap(const std::string_view name, const std::string_view basedir, bool alpha_masked)
{

	std::string diffuse_path;
	diffuse_path = basedir;
	if(basedir.find_last_of('/') != basedir.length()-1)
		diffuse_path.push_back('/');
	diffuse_path.append(name);

	assert(cache != nullptr);

	m_diffuse_map = default_diffuse;


	auto set_type = [this, alpha_masked](int nrChannels)
	{
		clear(flags, Opaque | Masked | Blended);
		if(nrChannels == 3) {
			if(alpha_masked)
				fmt::print(stderr, "[material] specified alpha masked, but texture does not have alpha channel!\n");
			flags |= Opaque;
		}
		if(nrChannels == 4) {
			if(alpha_masked)
				flags |= Masked;
			else
				flags |= Blended;
		}
	};

	if(auto res = cache->get(diffuse_path); res.to) {
		m_diffuse_map = res.to;
		set_type(res.num_channels);
		return;
	}

	int num_channels = 0;
	auto res = load_texture(diffuse_path, false, 0, &num_channels);
	if(res) {
		m_diffuse_map = res;
		set_type(num_channels);
		cache->add(std::move(diffuse_path), res, num_channels);
	} else {
		fmt::print(stderr, "[material] failed to load diffuse map from: [{}]\n", diffuse_path);
	}
}

void Material::addNormalMap(const std::string_view name, const std::string_view basedir)
{
	std::string normal_path;
	normal_path = basedir;
	if(basedir.find_last_of('/') != basedir.length()-1)
		normal_path.push_back('/');
	normal_path.append(name);

	assert(cache != nullptr);

	if(auto res = cache->get(normal_path); res.to) {
		m_normal_map = res.to;
		flags |= NormalMapped;
		return;
	}

	int num_channels = 0;
	auto res = load_texture(normal_path, true, 3, &num_channels);
	if(res) {
		m_normal_map = res;
		flags |= NormalMapped;
		cache->add(std::move(normal_path), res, num_channels);
	} else {
		fmt::print(stderr, "[material] failed to load normal map from: [{}]\n", normal_path);
	}
}

void Material::addSpecularMap(const std::string_view name, const std::string_view basedir)
{
	std::string specular_path;
	specular_path = basedir;
	if(basedir.find_last_of('/') != basedir.length()-1)
		specular_path.push_back('/');
	specular_path.append(name);

	assert(cache != nullptr);

	if(auto res = cache->get(specular_path); res.to) {
		m_specular_map = res.to;
		flags |= SpecularMapped;
		return;
	}

	int num_channels = 0;
	auto res = load_texture(specular_path, true, 0, &num_channels);
	if(res) {
		m_specular_map = res;
		flags |= SpecularMapped;
		cache->add(std::move(specular_path), res, num_channels);
	} else {
		fmt::print(stderr,"[material] failed to load specular map from: [{}]\n", specular_path);
	}
}

void TextureCache::add(std::string && path, uint32_t to, int numchan)
{
	cache.emplace(std::make_pair(std::move(path), Result{to, numchan}));
}

TextureCache::Result TextureCache::get(const std::string & path)
{
	const auto p = cache.find(path);
	if(p != cache.end())
		return p->second;
	return Result{};
}
