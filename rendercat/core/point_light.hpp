#pragma once
#include <rendercat/common.hpp>
#include <zcm/vec4.hpp>
#include <zcm/vec3.hpp>
#include <zcm/mat3.hpp>
#include <zcm/quat.hpp>
#include <zcm/angle_and_trigonometry.hpp>
#include <zcm/common.hpp>
#include <zcm/geometric.hpp>
#include <zcm/exponential.hpp>
#include <zcm/constants.hpp>
#include <cstdint>

namespace rc {

struct alignas(zcm::vec4) PunctualLightData
{
	zcm::vec3 position     = {0.0f, 0.0f, 0.0f};
	float     radius       = 5.0f;
	zcm::vec3 color        = {1.0f, 1.0f, 1.0f};
	float     intensity    = 11.9366f;
	zcm::quat orientation  = {0.0f, 0.0f, 1.0f, 0.0f};
	float     angle_scale  = 0.0f;
	float     angle_offset = 0.0f;
	float     m_angle_inn  = zcm::radians(20.0f); // pad to 16 bytes
	float     m_angle_out  = zcm::radians(30.0f);
};

static_assert(sizeof(PunctualLightData) % 16 == 0);
static_assert(alignof(PunctualLightData) == 16);


struct PointLight
{
	PunctualLightData data;

	enum State
	{
		NoState,
		Enabled             = 0x1,
		ShowWireframe       = 0x2,
		FollowCamera        = 0x4
	};

	std::uint8_t state = Enabled;

	zcm::vec3 position() const noexcept
	{
		return data.position;
	}

	void set_position(zcm::vec3 pos) noexcept
	{
		data.position = pos;
	}

	zcm::vec3 color() const noexcept
	{
		return data.color;
	}

	void set_color(zcm::vec3 col) noexcept
	{
		data.color = col;
	}

	float radius() const noexcept
	{
		return data.radius;
	}

	void set_radius(float r) noexcept
	{
		data.radius = r;
	}

	float flux() const noexcept
	{
		// ref: equation (15) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return data.intensity * 4.0f * zcm::pi();
	}

	void set_flux(float luminous_flux) noexcept // in lumens
	{
		data.intensity = zcm::max(luminous_flux, 0.1f) / (4.0f * zcm::pi());
	}

	float distance_attenuation(float dist) const noexcept
	{
		const float decay = 2.0f;

		// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return zcm::pow(zcm::clamp(1.0f - zcm::pow(dist/data.radius, 4.0f), 0.0f, 1.0f), decay) / zcm::max(zcm::pow(dist, decay), 0.01f);
	}
};

struct SpotLight final : public PointLight
{


	void update_angle_scale_offset() noexcept
	{
		float cosInn = zcm::cos(data.m_angle_inn);
		float cosOut = zcm::cos(data.m_angle_out);
		data.angle_scale = 1.0f / zcm::max(0.001f, cosInn - cosOut);
		data.angle_offset = (-cosOut) * data.angle_scale;
	}

public:
	SpotLight()
	{
		update_angle_scale_offset();
	}

	void set_orientation(zcm::quat dir) noexcept
	{
		data.orientation = dir;
	}

	zcm::quat orientation() const noexcept
	{
		return data.orientation;
	}

	zcm::vec3 direction_vec() const noexcept {
		return data.orientation * zcm::vec3{0.0f, 0.0f, 1.0f};
	}

	float flux() const noexcept
	{
		// ref: equation (17) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
		// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
		return data.intensity * zcm::pi();
	}

	void set_flux(float luminous_flux) noexcept // in lumens
	{
		data.intensity = zcm::max(luminous_flux, 1.0f) / zcm::pi();
	}

	void set_angle_outer(float angle) noexcept
	{
		data.m_angle_out = zcm::clamp(angle, 0.0f, zcm::radians(90.0f));
		update_angle_scale_offset();
	}

	float angle_outer() const noexcept
	{
		return data.m_angle_out;
	}

	void set_angle_inner(float angle) noexcept
	{
		data.m_angle_inn = zcm::clamp(angle, 0.0f, data.m_angle_out);
		update_angle_scale_offset();
	}

	float angle_inner() const noexcept
	{
		return data.m_angle_inn;
	}

	float angle_scale() const noexcept
	{
		return data.angle_scale;
	}

	float angle_offset() const noexcept
	{
		return data.angle_offset;
	}

	float direction_attenuation(zcm::vec3 pos) const noexcept
	{
		auto dir = zcm::normalize(data.position - pos);
		float att = zcm::clamp(zcm::dot(direction_vec(), dir) * data.angle_scale + data.angle_offset, 0.0f, 1.0f);
		return att * att;
	}
};

}
