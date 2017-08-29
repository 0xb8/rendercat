#include <rendercat/material.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/uniform.hpp>
#include <stb_image.h>
#include <iostream>
#include <cmath>

uint32_t Material::default_diffuse = 0;
TextureCache* Material::cache = nullptr;

GLuint load_texture(const std::string& path, bool linear, int desired_channels = 0, int* num_channels = nullptr)
{
	static const float max_aniso = 16.0f;
	static const int   max_levels = 14;

	static float maxAnisotropy = 0.0f;
	if(maxAnisotropy == 0.0f)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	GLuint tex = 0;
	int width, height, nrChannels;
	auto data = stbi_load(path.data(), &width, &height, &nrChannels, desired_channels);
	if (data) {
		int nr_levels = std::min((int)std::round(std::log2(std::min(width, height))), max_levels);

		glCreateTextures(GL_TEXTURE_2D, 1, &tex);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(maxAnisotropy, max_aniso));

		if(desired_channels == 0) desired_channels = 1000;

		auto format = (linear ? GL_RGB : GL_SRGB8);

		switch (std::min(nrChannels, desired_channels)) {
		case 1:
			static const GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
			glTextureParameteriv(tex, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			glTextureStorage2D(tex, nr_levels, format, width, height);
			glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data);
			break;
		case 3:
			glTextureStorage2D(tex, nr_levels, format, width, height);
			glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
			break;
		case 4:

			glTextureStorage2D(tex, nr_levels, (linear ? GL_RGBA : GL_SRGB8_ALPHA8), width, height);
			glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			break;
		default:
			std::cerr << "[material] invalid channel count: " << nrChannels << " in " << path << '\n';
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

void Material::bind(GLuint s) const noexcept
{
	unif::v3(s,  "material.specular",  m_specular_color);
	unif::f1(s,  "material.shininess", m_shininess);
	unif::i1(s,  "material.type",      flags);

	if(m_diffuse_map) {
		glBindTextureUnit(0, m_diffuse_map);
		//unif::i1(s, "material.diffuse", 0);
	} else {
		glBindTextureUnit(0,  0);
	}
	if(m_normal_map) {
		glBindTextureUnit(1,  m_normal_map);
		//unif::i1(s, "material.normal", 1);
	} else {
		glBindTextureUnit(1, 0);
	}
	if(m_specular_map) {
		glBindTextureUnit(2, m_specular_map);
		//unif::i1(s, "material.specular_map", 2);
	} else {
		glBindTextureUnit(2, 0);
	}
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
				std::cerr << "specified alpha masked, but texture does not have alpha channel!";
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
		std::cerr << " failed!";
		std::cerr << " Path: [" << diffuse_path << "]";
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
		std::cerr << " failed!";
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
		std::cerr << " failed!";
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
