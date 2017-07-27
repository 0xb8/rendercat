#pragma once

#include <common.hpp>

class PointLight
{
	glm::vec3 m_position;
	glm::vec3 m_ambient;
	glm::vec3 m_diffuse;
	glm::vec3 m_specular;
	float m_radius = 20.0f;
	float m_luminous_intensity = 20.0f; // in candelas

public:

	PointLight() = default;

	glm::vec3 position() const
	{
		return m_position;
	}
	PointLight& position(glm::vec3 pos)
	{
		m_position = pos;
		return *this;
	}

	glm::vec3 ambient() const
	{
		return m_ambient;
	}
	PointLight& ambient(glm::vec3 amb)
	{
		assert(m::saturated(amb));
		m_ambient = amb;
		return *this;
	}

	glm::vec3 diffuse() const
	{
		return m_diffuse;
	}
	PointLight& diffuse(glm::vec3 dif)
	{
		assert(m::saturated(dif));
		m_diffuse = dif;
		return *this;
	}

	glm::vec3 specular() const
	{
		return m_specular;
	}
	PointLight& specular(glm::vec3 spec)
	{
		assert(m::saturated(spec));
		m_specular = spec;
		return *this;
	}

	float radius() const
	{
		return m_radius;
	}

	PointLight& radius(float rad)
	{
		assert(rad > 0.0f && rad < 500.0f);
		m_radius = rad;
		return *this;
	}

	float intensity() const // in candelas
	{
		return m_luminous_intensity;
	}

	float flux() const
	{
		// ref: equation (15) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return m_luminous_intensity * 4.0 * mc::M_PI;
	}

	PointLight& flux(float luminous_flux) // in lumens
	{
		assert(luminous_flux > 0.0f);
		m_luminous_intensity = luminous_flux / (4.0 * mc::M_PI);
		return *this;
	}
};
