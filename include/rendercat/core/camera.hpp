#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/frustum.hpp>
#include <zcm/vec3.hpp>
#include <zcm/quat.hpp>
#include <zcm/angle_and_trigonometry.hpp>
#include <zcm/quaternion.hpp>
#include <zcm/geometric.hpp>
#include <zcm/mat4.hpp>

namespace rc {

struct Camera
{
	friend class Scene;

	void pitch(float pitchRadians, float limit = zcm::radians(89.0f)) noexcept
	{
		if (zcm::abs(pitchRadians + _pitch) > limit) {
			pitchRadians = zcm::radians(89.0f) - zcm::abs(_pitch);
			pitchRadians *= zcm::sign(_pitch);
		}
		rotate(pitchRadians, zcm::vec3(1.0f, 0.0f, 0.0f));
		_pitch += pitchRadians;
	}

	void yaw(float yawRadians) noexcept
	{
		rotate(yawRadians, zcm::vec3(0.0f, 1.0f, 0.0f));
	}

	void yaw_global(float turnRadians) noexcept
	{
		if(zcm::dot(get_up(), get_global_up()) < 0) {
			turnRadians *= -1.0f;
		}

		zcm::quat q = zcm::angleAxis(turnRadians, orientation * zcm::vec3(0.0f, 1.0f, 0.0f));
		rotate(q);
	}

	void roll(float rollRadians) noexcept
	{
		rotate(rollRadians, zcm::vec3(0.0f, 0.0f, 1.0f));
	}

	void rotate(float angleRadians, const zcm::vec3& axis) noexcept
	{
		zcm::quat q = zcm::angleAxis(angleRadians, axis);
		rotate(q);
	}

	void rotate(const zcm::quat& rotation) noexcept
	{
		orientation = zcm::normalize(rotation * orientation);
	}

	zcm::vec3 get_forward() const noexcept
	{
		return zcm::conjugate(orientation) * zcm::vec3(0.0f, 0.0f, -1.0f);
	}

	zcm::vec3 get_left() const noexcept
	{
		return zcm::conjugate(orientation) * zcm::vec3(-1.0, 0.0f, 0.0f);
	}

	zcm::vec3 get_up() const noexcept
	{
		return zcm::conjugate(orientation) * zcm::vec3(0.0f, 1.0f, 0.0f);
	}

	zcm::vec3 get_global_up() const noexcept
	{
		return zcm::vec3(0.0f, 1.0f, 0.0f);
	}

	zcm::vec3 get_global_left() const noexcept
	{
		return zcm::normalize(zcm::cross(get_global_up(), get_forward()));
	}

	void move_forward(float movement) noexcept
	{
		position += get_forward() * movement;
	}

	void move_left(float movement) noexcept
	{
		position += get_global_left() * movement;
	}

	void move_up(float movement) noexcept
	{
		position += get_global_up() * movement;
	}

	void set_fov(float newfov) noexcept
	{
		fov = newfov;
	}

	void set_aspect(float newaspect) noexcept
	{
		aspect = newaspect;
	}

	void zoom(float offset) noexcept
	{
		set_fov(zcm::clamp(fov + offset, fov_min, fov_max));
	}



	static constexpr float fov_min = 10.0f;
	static constexpr float fov_max = 110.0f;

	static constexpr float znear_min = 0.001f;

	static zcm::mat4 make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear) noexcept;
	void update() noexcept;


	zcm::mat4 view;
	zcm::mat4 projection;
	zcm::mat4 view_projection;

	zcm::quat orientation;
	zcm::vec3 position;

	float exposure = 1.0f;
	rc::Frustum frustum;

private:

	float _pitch = 0.f;
	float fov = 90.0f;
	float aspect = 1.0f;
	float znear = 0.01f;
	float zfar  = 10.0f; // not needed with reverse-z, kept for frustum far plane
};

}
