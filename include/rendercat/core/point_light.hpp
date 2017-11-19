#pragma once

#include <rendercat/common.hpp>

template<typename T>
class PunctualLight // CRTP base class
{
	glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
	glm::vec3 m_ambient  = {0.0f, 0.0f, 0.0f};
	glm::vec3 m_diffuse  = {1.0f, 1.0f, 1.0f};
	glm::vec3 m_specular = {0.1f, 0.1f, 0.1f};
	float m_radius = 5.0f;
	float m_luminous_intensity = 11.9366f; // in candelas, equiv. 150 lm
	float m_color_temperature = 6500.0f;   // in Kelvin, ranges from ~1000 to ~22000

public:
	enum State
	{
		NoState,
		Enabled             = 0x1,
		ShowWireframe       = 0x2,
		FollowCamera        = 0x4
	};

	uint8_t state = Enabled;

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
		m_ambient = glm::clamp(amb, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
		return *static_cast<T*>(this);
	}

	glm::vec3 diffuse() const noexcept
	{
		return m_diffuse;
	}
	T& diffuse(glm::vec3 dif) noexcept
	{
		m_diffuse = glm::clamp(dif, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});;
		return *static_cast<T*>(this);
	}

	glm::vec3 specular() const noexcept
	{
		return m_specular;
	}
	T& specular(glm::vec3 spec) noexcept
	{
		m_specular = glm::clamp(spec, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
		return *static_cast<T*>(this);
	}

	float color_temperature() const noexcept
	{
		return m_color_temperature;
	}

	T& color_temperature(float temp) noexcept
	{
		m_color_temperature = glm::clamp(temp, 273.0f, 22000.0f);
		return *static_cast<T*>(this);
	}

	float radius() const noexcept
	{
		return m_radius;
	}

	T& radius(float rad) noexcept
	{
		m_radius = glm::clamp(rad, 0.1f, 1000.0f);
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
		return m_luminous_intensity * 4.0 * rc::mathconst::pi;
	}

	T& flux(float luminous_flux) noexcept // in lumens
	{
		m_luminous_intensity = glm::clamp(luminous_flux, 0.1f, 20000.0f) / (4.0 * rc::mathconst::pi);
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
	float     m_angle_out = glm::radians(30.0f);
	float     m_angle_inn = glm::radians(15.0f);
	float     m_angle_scale = 0.0f;
	float     m_angle_offset = 0.0f;

	void update_angle_scale_offset() noexcept
	{
		float cosInn = glm::cos(m_angle_inn);
		float cosOut = glm::cos(m_angle_out);
		m_angle_scale= 1.0f / std::max(0.001f, cosInn - cosOut);
		m_angle_offset = (-cosOut) * m_angle_scale;
	}

public:
	SpotLight() : PunctualLight<SpotLight>()
	{
		update_angle_scale_offset();
	}

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
		return intensity() * rc::mathconst::pi;
	}

	SpotLight& flux(float luminous_flux) noexcept // in lumens
	{
		intensity(glm::clamp(luminous_flux, 1.0f, 20000.0f) /  rc::mathconst::pi);
		return *(this);
	}

	SpotLight& angle_outer(float angle) noexcept
	{
		m_angle_out = glm::clamp(angle, 0.0f, glm::radians(90.0f));
		update_angle_scale_offset();
		return *this;
	}

	float angle_outer() const noexcept
	{
		return m_angle_out;
	}

	SpotLight& angle_inner(float angle) noexcept
	{
		m_angle_inn = glm::clamp(angle, 0.0f, m_angle_out);
		update_angle_scale_offset();
		return *this;
	}

	float angle_inner() const noexcept
	{
		return m_angle_inn;
	}

	float angle_scale() const noexcept
	{
		return m_angle_scale;
	}

	float angle_offset() const noexcept
	{
		return m_angle_offset;
	}

	float direction_attenuation(glm::vec3 pos) const noexcept
	{
		auto dir = glm::normalize(position() - pos);
		float att = glm::clamp(glm::dot(m_direction, dir) * m_angle_scale + m_angle_offset, 0.0f, 1.0f);
		return att * att;

	}
};

