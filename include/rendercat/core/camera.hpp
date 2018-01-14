#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/frustum.hpp>

namespace rc {

struct Camera
{
	friend class Scene;

	void aim(float dx, float dy) noexcept;
	void forward(float speed) noexcept;
	void backward(float speed) noexcept;
	void left(float speed) noexcept;
	void right(float speed) noexcept;
	void zoom(float newfov) noexcept;
	void zoom_scroll_offset(float offset) noexcept;

	static constexpr float fov_min = 10.0f;
	static constexpr float fov_max = 110.0f;

	static constexpr float znear_min = 0.001f;

	static glm::mat4 make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear) noexcept;
	void update_view() noexcept;
	void update_projection(float newaspect) noexcept;

	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 view_projection;

	glm::vec3 pos   = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);

	float exposure = 1.0f;

	rc::Frustum frustum;

private:
	glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);

	float pitch = 0.0f;
	float yaw = -90.0f;
	float fov = 90.0f;
	float aspect = 1.0f;
	float znear = 0.01f;
	float zfar  = 10.0f; // not needed with reverse-z, kept for frustum far plane
};

}
