#pragma once
#include <rendercat/common.hpp>
#include <zcm/vec3.hpp>
#include <zcm/angle_and_trigonometry.hpp>
#include <zcm/common.hpp>
#include <zcm/geometric.hpp>
#include <zcm/exponential.hpp>
#include <zcm/constants.hpp>
#include <cstdint>

namespace rc {

template<typename T>
class PunctualLight // CRTP base class
{
	zcm::vec3 m_position = {0.0f, 0.0f, 0.0f};
	zcm::vec3 m_diffuse  = {1.0f, 1.0f, 1.0f};
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

	std::uint8_t state = Enabled;

	constexpr PunctualLight() = default;

	zcm::vec3 position() const noexcept
	{
		return m_position;
	}
	T& position(zcm::vec3 pos) noexcept
	{
		m_position = pos;
		return *static_cast<T*>(this);
	}

	zcm::vec3 diffuse() const noexcept
	{
		return m_diffuse;
	}
	T& diffuse(zcm::vec3 dif) noexcept
	{
		m_diffuse = zcm::clamp(dif, 0.0f, 1.0f);
		return *static_cast<T*>(this);
	}

	float color_temperature() const noexcept
	{
		return m_color_temperature;
	}

	T& color_temperature(float temp) noexcept
	{
		m_color_temperature = zcm::clamp(temp, 273.0f, 22000.0f);
		return *static_cast<T*>(this);
	}

	float radius() const noexcept
	{
		return m_radius;
	}

	T& radius(float rad) noexcept
	{
		m_radius = zcm::clamp(rad, 0.1f, 1000.0f);
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
		return m_luminous_intensity * 4.0 * zcm::pi();
	}

	T& flux(float luminous_flux) noexcept // in lumens
	{
		m_luminous_intensity = zcm::clamp(luminous_flux, 0.1f, 20000.0f) / (4.0 * zcm::pi());
		return *static_cast<T*>(this);
	}

	float distance_attenuation(float dist) const noexcept
	{
		const float decay = 2.0f;

		// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return zcm::pow(zcm::clamp(1.0 - zcm::pow(dist/m_radius, 4.0), 0.0, 1.0), decay) / zcm::max(zcm::pow(dist, decay), 0.01f);
	}
};

struct PointLight final : public PunctualLight<PointLight>
{
	constexpr PointLight() = default;
};


struct SpotLight final : public PunctualLight<SpotLight>
{
	zcm::vec3 m_direction {0.0f, 1.0f, 0.0f};
	float     m_angle_out = zcm::radians(30.0f);
	float     m_angle_inn = zcm::radians(15.0f);
	float     m_angle_scale = 0.0f;
	float     m_angle_offset = 0.0f;

	void update_angle_scale_offset() noexcept
	{
		float cosInn = zcm::cos(m_angle_inn);
		float cosOut = zcm::cos(m_angle_out);
		m_angle_scale= 1.0f / zcm::max(0.001f, cosInn - cosOut);
		m_angle_offset = (-cosOut) * m_angle_scale;
	}

public:
	SpotLight() : PunctualLight<SpotLight>()
	{
		update_angle_scale_offset();
	}

	SpotLight& direction(zcm::vec3 dir) noexcept
	{
		m_direction = zcm::normalize(dir);
		return *this;
	}

	zcm::vec3 direction() const noexcept
	{
		return m_direction;
	}

	float flux() const noexcept
	{
		// ref: equation (17) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return intensity() * zcm::pi();
	}

	SpotLight& flux(float luminous_flux) noexcept // in lumens
	{
		intensity(zcm::clamp(luminous_flux, 1.0f, 20000.0f) / zcm::pi());
		return *(this);
	}

	SpotLight& angle_outer(float angle) noexcept
	{
		m_angle_out = zcm::clamp(angle, 0.0f, zcm::radians(90.0f));
		update_angle_scale_offset();
		return *this;
	}

	float angle_outer() const noexcept
	{
		return m_angle_out;
	}

	SpotLight& angle_inner(float angle) noexcept
	{
		m_angle_inn = zcm::clamp(angle, 0.0f, m_angle_out);
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

	float direction_attenuation(zcm::vec3 pos) const noexcept
	{
		auto dir = zcm::normalize(position() - pos);
		float att = zcm::clamp(zcm::dot(m_direction, dir) * m_angle_scale + m_angle_offset, 0.0f, 1.0f);
		return att * att;

	}
};

}
