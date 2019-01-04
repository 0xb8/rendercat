#include <rendercat/core/camera.hpp>
#include <zcm/matrix_transform.hpp>
#include <zcm/mat3.hpp>
#include <zcm/matrix_transform.hpp>
#include <zcm/matrix.hpp>
#include <zcm/constants.hpp>
#include <zcm/quaternion.hpp>
#include <zcm/angle_and_trigonometry.hpp>
#include <zcm/quaternion.hpp>
#include <zcm/geometric.hpp>
#include <zcm/common.hpp>
#include <fmt/core.h>

static zcm::mat4 make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear) noexcept
{
	float f = 1.0f / zcm::tan(fovY_radians / 2.0f);
	float g = f / aspectWbyH;
	return zcm::mat4(g,    0.0f,  0.0f,  0.0f,
	                 0.0f,    f,  0.0f,  0.0f,
	                 0.0f, 0.0f,  0.0f, -1.0f,
	                 0.0f, 0.0f, zNear,  0.0f);
}

static zcm::vec3 calc_turntable_pos(float distance, zcm::vec3 target, const zcm::quat& orientation)
{
	auto shift_vec = zcm::vec3{0.0f, 0.0f, 1.0f};
	shift_vec *= distance;

	shift_vec = shift_vec * orientation;
	return target + shift_vec;
}

zcm::mat4 rc::make_projection(const CameraState& state) noexcept
{
	return make_reversed_z_projection(state.fov, state.aspect, state.znear);
}

zcm::mat4 rc::make_view(rc::CameraState & state) noexcept
{
	return zcm::translate(zcm::mat4_cast(state.orientation), -state.position);
}

void rc::TurntableCameraBehavior::update(rc::CameraState& state) noexcept
{
	state.position = calc_turntable_pos(distance, target, state.orientation);
}

void rc::TurntableCameraBehavior::closer(rc::CameraState& state, float offset) noexcept
{
	distance = zcm::max(distance + offset, 0.0f);
	update(state);
}

void rc::TurntableCameraBehavior::on_mouse_move(rc::CameraState & state, float xoffset, float yoffset) {

	/* original turntable view code by John Aughey for Blender project */
	auto global_up = state.get_global_up();
	auto cam_right = state.get_right();
	auto cam_up    = state.get_up();
	zcm::vec3 xaxis;

	/* Sensitivity will control how fast the viewport rotates.  0.007 was
	 * obtained experimentally by looking at viewport rotation sensitivities
	 * on other modeling programs. */
	/* Perhaps this should be a configurable user parameter. */
	const float sensitivity = 0.014f;

	auto angle_norm = [](zcm::vec3 v1, zcm::vec3 v2)
	{
		auto clamped_asin = [](float fac)
		{
			if      (unlikely(fac <= -1.0f)) return -zcm::half_pi();
			else if (unlikely(fac >=  1.0f)) return zcm::half_pi();
			else return zcm::asin(fac);
		};

		if (zcm::dot(v1, v2) >= 0.0f) {
			return 2.0f * clamped_asin(zcm::distance(v1, v2) * 0.5f);
		} else {
			return zcm::pi() - 2.0f * clamped_asin(zcm::distance(v1, -v2) * 0.5f);
		}
	};

	/* avoid gimble lock */
	if (likely(zcm::length2(global_up - cam_up) > 0.001f)) {
		float fac;
		xaxis = zcm::cross(global_up, cam_up);
		if (zcm::dot(xaxis, cam_right) < 0) {
			xaxis *= -1.f;
		}
		fac = angle_norm(global_up, cam_up) / zcm::pi();
		fac = zcm::abs(fac - 0.5f) * 2;
		fac = fac * fac;
		xaxis = zcm::mix(xaxis, cam_right, fac);
	} else {
		xaxis = cam_right;
	}

	/* Perform the up/down rotation */
	auto quat_local_x = state.orientation * zcm::angleAxis(sensitivity * yoffset, xaxis);

	/* perform the 'turntable' rotation */
	auto quat_global_y = zcm::angleAxis(sensitivity * xoffset, zcm::vec3{0.0f, 1.f, 0.0f});

	state.orientation = quat_local_x * quat_global_y;
	update(state);
}

void rc::TurntableCameraBehavior::move_forward(rc::CameraState & state, float movement) noexcept
{
	auto delta = state.get_forward() * movement;
	state.position += delta;
	target += delta;
}

void rc::TurntableCameraBehavior::move_right(rc::CameraState & state, float movement) noexcept
{
	auto delta = state.get_global_right() * movement;
	state.position += delta;
	target += delta;
}

void rc::TurntableCameraBehavior::move_up(rc::CameraState & state, float movement) noexcept
{
	auto delta = state.get_global_up() * movement;
	state.position += delta;
	target += delta;
}

void rc::FPSCameraBehavior::update(rc::CameraState&) noexcept
{
	// nothing
}

void rc::FPSCameraBehavior::rotate(rc::CameraState & state, const zcm::quat & rotation) noexcept
{
	state.orientation = zcm::normalize(rotation * state.orientation);
}

void rc::FPSCameraBehavior::rotate(rc::CameraState & state, float angleRadians, const zcm::vec3 & axis) noexcept
{
	zcm::quat q = zcm::angleAxis(angleRadians, axis);
	rotate(state, q);
}

void rc::FPSCameraBehavior::pitch(rc::CameraState & state, float pitchRadians, float limit) noexcept
{
	if (zcm::abs(pitchRadians + _pitch) > limit) {
		pitchRadians = zcm::radians(89.0f) - zcm::abs(_pitch);
		pitchRadians *= zcm::sign(_pitch);
	}
	rotate(state, pitchRadians, zcm::vec3(1.0f, 0.0f, 0.0f));
	_pitch += pitchRadians;
}

void rc::FPSCameraBehavior::yaw(rc::CameraState & state, float yawRadians) noexcept
{
	rotate(state, yawRadians, zcm::vec3(0.0f, 1.0f, 0.0f));
}

void rc::FPSCameraBehavior::yaw_global(rc::CameraState & state, float turnRadians) noexcept
{
	if(zcm::dot(state.get_up(), state.get_global_up()) < 0) {
		turnRadians *= -1.0f;
	}

	zcm::quat q = zcm::angleAxis(turnRadians, state.orientation * zcm::vec3(0.0f, 1.0f, 0.0f));
	rotate(state, q);
}

void rc::FPSCameraBehavior::on_mouse_move(rc::CameraState & state, float xoffset, float yoffset) {
	pitch(state, zcm::radians(yoffset));
	yaw_global(state, zcm::radians(xoffset));
}

void rc::FPSCameraBehavior::roll(rc::CameraState & state, float rollRadians) noexcept
{
	rotate(state, rollRadians, zcm::vec3(0.0f, 0.0f, 1.0f));
}

void rc::CameraState::normalize() noexcept
{
	orientation = zcm::normalize(orientation);
}

zcm::vec3 rc::CameraState::get_forward() const noexcept
{
	// unit quat inverse == conjugate
	return zcm::conjugate(orientation) * zcm::vec3(0.0f, 0.0f, -1.0f);
}

zcm::vec3 rc::CameraState::get_right() const noexcept
{
	// unit quat inverse == conjugate
	return zcm::conjugate(orientation) * zcm::vec3(1.0, 0.0f, 0.0f);
}

zcm::vec3 rc::CameraState::get_up() const noexcept
{
	// unit quat inverse == conjugate
	return zcm::conjugate(orientation) * zcm::vec3(0.0f, 1.0f, 0.0f);
}

zcm::vec3 rc::CameraState::get_global_forward() const noexcept
{
	return zcm::normalize(zcm::cross(get_global_up(), zcm::cross(get_forward(), get_global_up())));
}

zcm::vec3 rc::CameraState::get_global_up() const noexcept
{
	return zcm::vec3(0.0f, 1.0f, 0.0f);
}

zcm::vec3 rc::CameraState::get_global_right() const noexcept
{
	return zcm::normalize(zcm::cross(get_forward(), get_global_up()));
}

void rc::ZoomCameraBehavior::zoom(rc::CameraState & state, float offset) noexcept
{
	state.fov = zcm::radians(zcm::clamp(zcm::degrees(state.fov) + offset, fov_min, fov_max));
}

void rc::FPSCameraBehavior::move_forward(rc::CameraState & state, float movement) noexcept
{
	state.position += state.get_forward() * movement;
}

void rc::FPSCameraBehavior::move_right(rc::CameraState & state, float movement) noexcept
{
	state.position += state.get_global_right() * movement;
}

void rc::FPSCameraBehavior::move_up(rc::CameraState & state, float movement) noexcept
{
	state.position += state.get_global_up() * movement;
}

void rc::Camera::update() noexcept
{
	state.normalize();
	behavior.update(state);
	frustum.update(state);
}
