#pragma once

#include <rendercat/common.hpp>

template<typename T>
class PunctualLight // CRTP base class
{
	glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
	glm::vec3 m_ambient  = {0.0f, 0.0f, 0.0f};
	glm::vec3 m_diffuse  = {1.0f, 1.0f, 1.0f};
	glm::vec3 m_specular = {0.1f, 0.1f, 0.1f};
	float m_radius = 10.0f;
	float m_luminous_intensity = 11.9366f; // in candelas, equiv. 150 lm

public:
	bool enabled = true;

	constexpr PunctualLight() = default;

	glm::vec3 position() const noexcept
	{
		return m_position;
	}
	T& position(glm::vec3 pos) noexcept
	{
		m_position = pos;
		return *static_cast<T*>(this);
	}

	glm::vec3 ambient() const noexcept
	{
		return m_ambient;
	}
	T& ambient(glm::vec3 amb) noexcept
	{
		if(m::between(amb, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}))
			m_ambient = amb;
		return *static_cast<T*>(this);
	}

	glm::vec3 diffuse() const noexcept
	{
		return m_diffuse;
	}
	T& diffuse(glm::vec3 dif) noexcept
	{
		if(m::between(dif, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}))
			m_diffuse = dif;
		return *static_cast<T*>(this);
	}

	glm::vec3 specular() const noexcept
	{
		return m_specular;
	}
	T& specular(glm::vec3 spec) noexcept
	{
		if(m::between(spec, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}))
			m_specular = spec;
		return *static_cast<T*>(this);
	}

	float radius() const noexcept
	{
		return m_radius;
	}

	T& radius(float rad) noexcept
	{
		if(rad >= 0.0f && rad < 1000.0f)
			m_radius = rad;

		return *static_cast<T*>(this);
	}

	float intensity() const noexcept // in candelas
	{
		return m_luminous_intensity;
	}

	T& intensity(float intensity) noexcept
	{
		m_luminous_intensity = intensity;
		return *static_cast<T*>(this);
	}

	float flux() const noexcept
	{
		// ref: equation (15) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return m_luminous_intensity * 4.0 * mc::pi;
	}

	T& flux(float luminous_flux) noexcept // in lumens
	{
		if(luminous_flux > 0.0f && luminous_flux < 20000.0f)
			m_luminous_intensity = luminous_flux / (4.0 * mc::pi);
		return *static_cast<T*>(this);
	}

	float distance_attenuation(float dist) const noexcept
	{
		const float decay = 2.0f;

		// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return glm::pow(glm::clamp(1.0 - glm::pow(dist/m_radius, 4.0), 0.0, 1.0), decay) / glm::max(glm::pow(dist, decay), 0.01f);
	}
};

struct PointLight final : public PunctualLight<PointLight>
{
	constexpr PointLight() = default;
};


struct SpotLight final : public PunctualLight<SpotLight>
{
	glm::vec3 m_direction {0.0f, 1.0f, 0.0f};
	float	  m_angle_out = glm::radians(25.0f);
	float	  m_angle_inn = glm::radians(12.5f);

public:
	SpotLight() = default;

	SpotLight& direction(glm::vec3 dir) noexcept
	{
		m_direction = glm::normalize(dir);
		return *this;
	}

	glm::vec3 direction() const noexcept
	{
		return m_direction;
	}

	float flux() const noexcept
	{
		// ref: equation (17) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return intensity() * mc::pi;
	}

	SpotLight& flux(float luminous_flux) noexcept // in lumens
	{
		if(luminous_flux > 0.0f && luminous_flux < 20000.0f) {
			intensity(luminous_flux /  mc::pi);
		}
		return *(this);
	}

	SpotLight& angle_outer(float angle) noexcept
	{
		if(!glm::isinf(angle) && !glm::isnan(angle))
			m_angle_out = glm::clamp(angle, 0.0f, glm::radians(90.0f));
		return *this;
	}

	float angle_outer() const noexcept
	{
		return m_angle_out;
	}

	SpotLight& angle_inner(float angle) noexcept
	{
		if(!glm::isinf(angle) && !glm::isnan(angle))
			m_angle_inn = glm::clamp(angle, 0.0f, m_angle_out);
		return *this;
	}

	float angle_inner() const noexcept
	{
		return m_angle_inn;
	}

	std::pair<float,float> angle_scale_offset() const noexcept
	{
		float cosInn = glm::cos(m_angle_inn);
		float cosOut = glm::cos(m_angle_out);
		float scale = 1.0f / std::max(0.001f, cosInn - cosOut);
		float offset = (-cosOut) * scale;
		return std::make_pair(scale, offset);
	}

	float direction_attenuation(glm::vec3 pos) const noexcept
	{
		auto dir = glm::normalize(position() - pos);
		auto ang_sc = angle_scale_offset();
		float att = glm::clamp(glm::dot(m_direction, dir) * ang_sc.first + ang_sc.second, 0.0f, 1.0f);
		return att * att;

	}
};

