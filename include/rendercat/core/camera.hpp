#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/frustum.hpp>

namespace rc {

struct Camera
{
	friend class Scene;

	void pitch(float pitchRadians, float limit = glm::radians(89.0f))
	{
		if (std::abs(pitchRadians + _pitch) > limit) {
			pitchRadians = glm::radians(89.0f) - std::abs(_pitch);
			pitchRadians *= glm::sign(_pitch);
		}
		rotate(pitchRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		_pitch += pitchRadians;
	}

	void yaw(float yawRadians)
	{
		rotate(yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void yaw_global(float turnRadians)
	{

		if(glm::dot(get_up(), get_global_up()) < 0) {
			turnRadians *= -1.0f;
		}

		glm::quat q = glm::angleAxis(turnRadians, orientation * glm::vec3(0.0f, 1.0f, 0.0f));
		rotate(q);
	}

	void roll(float rollRadians)
	{
		rotate(rollRadians, glm::vec3(0.0f, 0.0f, 1.0f));
	}

	void rotate(float angleRadians, const glm::vec3& axis)
	{
		glm::quat q = glm::angleAxis(angleRadians, axis);
		rotate(q);
	}

	void rotate(const glm::quat& rotation)
	{
		orientation = rotation * orientation;
	}

	glm::vec3 get_forward() const
	{
		return glm::conjugate(orientation) * glm::vec3(0.0f, 0.0f, -1.0f);
	}

	glm::vec3 get_left() const
	{
		return glm::conjugate(orientation) * glm::vec3(-1.0, 0.0f, 0.0f);
	}

	glm::vec3 get_up() const
	{
		return glm::conjugate(orientation) * glm::vec3(0.0f, 1.0f, 0.0f);
	}

	glm::vec3 get_global_up() const
	{
		return glm::vec3(0.0f, 1.0f, 0.0f);
	}

	glm::vec3 get_global_left() const
	{
		return glm::cross(get_global_up(), get_forward());
	}

	void move_forward(float movement)
	{
		position += get_forward() * movement;
	}

	void move_left(float movement)
	{
		position += get_global_left() * movement;
	}

	void move_up(float movement)
	{
		position += get_global_up() * movement;
	}

	void set_fov(float newfov) noexcept
	{
		fov = newfov;
		projection = make_reversed_z_projection(glm::radians(fov), aspect, znear);
	}

	void zoom(float offset) noexcept
	{
		set_fov(glm::clamp(fov + offset, fov_min, fov_max));
	}

	static constexpr float fov_min = 10.0f;
	static constexpr float fov_max = 110.0f;

	static constexpr float znear_min = 0.001f;

	static glm::mat4 make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear) noexcept;
	void update_view() noexcept;
	void update_projection(float newaspect) noexcept;

	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 view_projection;

	glm::quat orientation;
	glm::vec3 position;

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
