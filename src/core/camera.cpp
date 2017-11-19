#include <rendercat/core/camera.hpp>

using namespace rc;

void Camera::aim(float dx, float dy) noexcept
{
	yaw += dx;
	pitch += dy;
	pitch = glm::clamp(pitch, -89.9f, 89.9f);
	yaw = fmod(yaw, 360.0);
	if(yaw < 0)
		yaw += 360.0;

	front = glm::normalize(glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
	                                 sin(glm::radians(pitch)),
	                                 sin(glm::radians(yaw)) * cos(glm::radians(pitch))));
	update_view();
}

void Camera::forward(float speed) noexcept
{
	pos += speed * front;
	update_view();
}

void Camera::backward(float speed) noexcept
{
	pos -= speed * front;
	update_view();
}

void Camera::left(float speed) noexcept
{
	pos -= glm::normalize(glm::cross(front, up)) * speed;
	update_view();
}

void Camera::right(float speed) noexcept
{
	pos += glm::normalize(glm::cross(front, up)) * speed;
	update_view();
}

void Camera::zoom(float newfov) noexcept
{
	fov = newfov;
	projection = make_reversed_z_projection(glm::radians(fov), aspect, znear);
}

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
	view = glm::lookAt(pos, pos + front, up);
	view_projection = projection * view;
	frustum.update(pos, front, up, glm::radians(fov), aspect, znear, zfar);
}

void Camera::update_projection(float newaspect) noexcept
{
	aspect = newaspect;
	projection = make_reversed_z_projection(glm::radians(fov), aspect, znear);
	view_projection = projection * view;
	frustum.update(pos, front, up, glm::radians(fov), aspect, znear, zfar);
}
