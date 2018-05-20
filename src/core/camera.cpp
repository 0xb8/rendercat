#include <rendercat/core/camera.hpp>

using namespace rc;

glm::mat4 Camera::make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear) noexcept
{
	float f = 1.0f / std::tan(fovY_radians / 2.0f);
	float g = f / aspectWbyH;
	return glm::mat4(g,    0.0f,  0.0f,  0.0f,
	                 0.0f,    f,  0.0f,  0.0f,
	                 0.0f, 0.0f,  0.0f, -1.0f,
	                 0.0f, 0.0f, zNear,  0.0f);
}

void Camera::update_view() noexcept
{
	orientation = glm::normalize(orientation);
	view = glm::translate(glm::mat4_cast(orientation), -position);
	view_projection = projection * view;
	frustum.update(position, get_forward(), get_up(), glm::radians(fov), aspect, znear, zfar);
}

void Camera::update_projection(float newaspect) noexcept
{
	aspect = newaspect;
	projection = make_reversed_z_projection(glm::radians(fov), aspect, znear);
	view_projection = projection * view;
	frustum.update(position, get_forward(), get_up(), glm::radians(fov), aspect, znear, zfar);
}
