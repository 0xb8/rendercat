#include <rendercat/core/camera.hpp>
#include <zcm/matrix_transform.hpp>

using namespace rc;

zcm::mat4 Camera::make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear) noexcept
{
	float f = 1.0f / zcm::tan(fovY_radians / 2.0f);
	float g = f / aspectWbyH;
	return zcm::mat4(g,    0.0f,  0.0f,  0.0f,
	                 0.0f,    f,  0.0f,  0.0f,
	                 0.0f, 0.0f,  0.0f, -1.0f,
	                 0.0f, 0.0f, zNear,  0.0f);
}

void Camera::update() noexcept
{
	orientation = zcm::normalize(orientation);
	view = zcm::translate(zcm::mat4_cast(orientation), -position);
	projection = make_reversed_z_projection(zcm::radians(fov), aspect, znear);
	view_projection = projection * view;
	frustum.update(position, get_forward(), get_up(), zcm::radians(fov), aspect, znear, zfar);
}

