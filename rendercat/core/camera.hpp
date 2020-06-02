#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/frustum.hpp>
#include <zcm/vec3.hpp>
#include <zcm/quat.hpp>
#include <zcm/mat4.hpp>


namespace rc {

zcm::mat4 make_projection_non_reversed_z(const CameraState& state) noexcept;
zcm::mat4 make_projection(const CameraState& state) noexcept;
zcm::mat4 make_view(const CameraState& state) noexcept;

struct CameraState
{
	zcm::quat orientation;
	zcm::vec3 position;

	float     exposure = 1.0f;
	float     fov = 1.5707963268f; // 90 degrees
	float     aspect = 1.0f;
	float     znear = 0.01f;
	float     zfar  = 10.0f; // not needed with reverse-z, kept for frustum far plane

	static constexpr float znear_min = 0.001f;

	/// Normalizes the orientation quaternion.
	/// Must be done at least once per frame and after any orientation change.
	void normalize() noexcept;

	/// Returns normalized forward vector.
	zcm::vec3 get_forward() const noexcept;

	/// Returns normalized right vector.
	zcm::vec3 get_right() const noexcept;

	/// Returns normalized up vector.
	zcm::vec3 get_up() const noexcept;

	/// Returns normalized forward vector wrt. global up vector.
	zcm::vec3 get_global_forward() const noexcept;

	/// Returns normalized right vector wrt. global up vector.
	zcm::vec3 get_global_right() const noexcept;

	/// Returns normalized world up vector (ie {0, 1, 0})
	zcm::vec3 get_global_up() const noexcept;
};


struct ZoomCameraBehavior
{
	static constexpr float fov_min = 10.0f;
	static constexpr float fov_max = 110.0f;

	void zoom(CameraState& state, float offset) noexcept;
};

struct FPSCameraBehavior  : public ZoomCameraBehavior
{
	float _pitch = 0.f;

	void update(CameraState& state) noexcept;

	void rotate(CameraState& state, const zcm::quat& rotation) noexcept;
	void rotate(CameraState& state, float angleRadians, const zcm::vec3& axis) noexcept;

	void pitch(CameraState& state, float pitchRadians, float limit = 1.553f) noexcept;
	void yaw(CameraState& state, float yawRadians) noexcept;
	void yaw_global(CameraState& state, float turnRadians) noexcept;
	void roll(CameraState& state, float rollRadians) noexcept;

	void on_mouse_move(CameraState& state, float xoffset, float yoffset);

	void move_forward(CameraState& state, float amount) noexcept;
	void move_right(CameraState& state, float amount) noexcept;
	void move_up(CameraState& state, float amount) noexcept;
};

struct TurntableCameraBehavior : public ZoomCameraBehavior
{
	zcm::vec3 target;
	float     distance = 0.25f;

	void update(CameraState& state) noexcept;

	void on_mouse_move(CameraState& state, float xoffset, float yoffset);

	void closer(CameraState& state, float offset) noexcept;
	void move_forward(CameraState& state, float movement) noexcept;
	void move_right(CameraState& state, float movement) noexcept;

	void move_up(CameraState& state, float movement) noexcept;
};


struct Camera
{
	void update() noexcept;

	CameraState               state;
	FPSCameraBehavior         behavior;
	rc::Frustum               frustum;
};

}
