#pragma once

#include <uniform.hpp>

class Material
{
	friend class Scene;

	static uint32_t default_diffuse;

	glm::vec3 specular_color = {0.0f, 0.0f, 0.0f};
	float     shininess      = 0.0f;

	uint32_t  diffuse_map  = 0;
	uint32_t  normal_map   = 0;
	uint32_t  specular_map = 0;
	bool      opaque       = true;

public:
	static void set_default_diffuse(uint32_t diffuse) noexcept
	{
		assert(diffuse);
		default_diffuse = diffuse;
	}

	Material() noexcept
	{
		diffuse_map = default_diffuse;
	}

	~Material() {
		glDeleteTextures(1, &diffuse_map);
		glDeleteTextures(1, &normal_map);
		glDeleteTextures(1, &specular_map);
	}

	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;

	Material(Material&& o) noexcept
	{
		this->operator=(std::move(o));
	}
	Material& operator =(Material&& o) noexcept
	{
		specular_color = o.specular_color;
		shininess      = o.shininess;
		diffuse_map    = o.diffuse_map;
		normal_map     = o.normal_map;
		specular_map   = o.specular_map;
		opaque         = o.opaque;

		o.diffuse_map  = 0;
		o.normal_map   = 0;
		o.specular_map = 0;
		return *this;
	}

	void bind(GLuint s) const noexcept
	{
		unif::v3(s,  "material.specular",  specular_color);
		unif::f1(s,  "material.shininess", shininess);
		if(diffuse_map) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuse_map);
			unif::i1(s, "material.diffuse", 0);
		} else {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		if(normal_map) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, normal_map);
			unif::i1(s, "material.normal", 1);
			unif::i1(s, "material.has_normal_map", 1);
		} else {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
			unif::i1(s, "material.has_normal_map", 0);
		}
		if(specular_map) {
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, specular_map);
			unif::i1(s, "material.specular_map", 2);
			unif::i1(s, "material.has_specular_map", 1);
		} else {
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, 0);
			unif::i1(s, "material.has_specular_map", 0);
		}
	}
};
