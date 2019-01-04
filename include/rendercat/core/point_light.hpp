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

struct PointLight
{
	zcm::vec3 position = {0.0f, 0.0f, 0.0f};
	zcm::vec3 diffuse  = {1.0f, 1.0f, 1.0f};
	float radius = 5.0f;
	float luminous_intensity = 11.9366f; // in candelas, equiv. 150 lm
	float color_temperature = 6500.0f;   // in Kelvin, ranges from ~1000 to ~22000

	enum State
	{
		NoState,
		Enabled             = 0x1,
		ShowWireframe       = 0x2,
		FollowCamera        = 0x4
	};

	std::uint8_t state = Enabled;

	float flux() const noexcept
	{
		// ref: equation (15) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return luminous_intensity * 4.0 * zcm::pi();
	}

	void set_flux(float luminous_flux) noexcept // in lumens
	{
		luminous_intensity = zcm::max(luminous_flux, 0.1f) / (4.0 * zcm::pi());
	}

	float distance_attenuation(float dist) const noexcept
	{
		const float decay = 2.0f;

		// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return zcm::pow(zcm::clamp(1.0 - zcm::pow(dist/radius, 4.0), 0.0, 1.0), decay) / zcm::max(zcm::pow(dist, decay), 0.01f);
	}
};

struct SpotLight final : public PointLight
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
	SpotLight()
	{
		update_angle_scale_offset();
	}

	void set_direction(zcm::vec3 dir) noexcept
	{
		m_direction = zcm::normalize(dir);
	}

	zcm::vec3 direction() const noexcept
	{
		return m_direction;
	}

	float flux() const noexcept
	{
		// ref: equation (17) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return luminous_intensity * zcm::pi();
	}

	void set_flux(float luminous_flux) noexcept // in lumens
	{
		luminous_intensity = zcm::max(luminous_flux, 1.0f) / zcm::pi();
	}

	void set_angle_outer(float angle) noexcept
	{
		m_angle_out = zcm::clamp(angle, 0.0f, zcm::radians(90.0f));
		update_angle_scale_offset();
	}

	float angle_outer() const noexcept
	{
		return m_angle_out;
	}

	void set_angle_inner(float angle) noexcept
	{
		m_angle_inn = zcm::clamp(angle, 0.0f, m_angle_out);
		update_angle_scale_offset();
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
		auto dir = zcm::normalize(position - pos);
		float att = zcm::clamp(zcm::dot(m_direction, dir) * m_angle_scale + m_angle_offset, 0.0f, 1.0f);
		return att * att;

	}
};

}
