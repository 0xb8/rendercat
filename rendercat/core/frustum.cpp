#include <rendercat/core/frustum.hpp>
#include <rendercat/core/camera.hpp>
#include <debug_draw.hpp>
#include <zcm/geometric.hpp>
#include <zcm/angle_and_trigonometry.hpp>
#include <zcm/common.hpp>
#include <zcm/constants.hpp>
#include <cassert>

using namespace rc;

enum side {
	NEAR = 0,
	LEFT = 1,
	RIGHT = 2,
	TOP = 3,
	BOTTOM = 4
};

enum point_pos {
	NEAR_TOP_LEFT,
	NEAR_TOP_RIGHT,
	NEAR_BOTTOM_LEFT,
	NEAR_BOTTOM_RIGHT,
	FAR_TOP_LEFT,
	FAR_TOP_RIGHT,
	FAR_BOTTOM_LEFT,
	FAR_BOTTOM_RIGHT
};

Plane::Plane(const zcm::vec3 & _normal, const zcm::vec3 & _point) noexcept
{
	const auto normal = zcm::normalize(_normal);
	plane = zcm::vec4(normal, -zcm::dot(normal, _point));
}

Plane::Plane(const zcm::vec3 & a, const zcm::vec3 & b, const zcm::vec3 & c) noexcept
{
	const auto normal = zcm::normalize(zcm::cross(b  - a, c - a));
	plane = zcm::vec4(normal, -zcm::dot(normal, a));
}

float Plane::distance(const zcm::vec3 & p) const noexcept
{
	return plane.w + zcm::dot({plane.x, plane.y, plane.z}, p);
}

void Frustum::update(const CameraState& cam_state) noexcept
{
	if(state & Locked) return;

	auto forward = -cam_state.get_backward();
	auto up      = cam_state.get_up();
	auto right   = cam_state.get_right();

	update(cam_state.position, forward, up, right, cam_state.fov, cam_state.aspect, cam_state.znear, cam_state.zfar);


}

void Frustum::update(const zcm::vec3 & position, const zcm::vec3 & forward, const zcm::vec3 & up, const zcm::vec3 & right, float fov,  float aspect, float znear, float zfar)
{
	assert(zcm::abs(zcm::length2(forward) - 1.0f) < 1e-6f); // check if camera quat was properly normalized
	assert(zcm::abs(zcm::length2(up) - 1.0f) < 1e-6f);
	assert(zcm::abs(zcm::length2(right) - 1.0f) < 1e-6f);

	near_center = position + forward * znear;
	far_center  = position + forward * zfar;

	auto near_height  = 2.0f * zcm::tan(fov * 0.5f) * znear;
	auto near_width = near_height * aspect;
	auto far_height   = 2.0f * zcm::tan(fov * 0.5f) * zfar;
	auto far_width  = far_height * aspect;

	far_width *= 0.5f;
	far_height *= 0.5f;
	near_width *= 0.5f;
	near_height *= 0.5f;

	auto far_top_left     = far_center + up * far_height + right * far_width;
	auto far_top_right    = far_center + up * far_height - right * far_width;
	auto far_bottom_left  = far_center - up * far_height + right * far_width;
	auto far_bottom_right = far_center - up * far_height - right * far_width;

	auto near_top_left     = near_center + up * near_height + right * near_width;
	auto near_top_right    = near_center + up * near_height - right * near_width;
	auto near_bottom_left  = near_center - up * near_height + right * near_width;
	auto near_bottom_right = near_center - up * near_height - right * near_width;

	planes[LEFT]   = Plane(near_bottom_left, far_top_left, far_bottom_left);
	planes[RIGHT]  = Plane(near_top_right, far_bottom_right, far_top_right);
	planes[TOP]    = Plane(near_top_left,far_top_right, far_top_left);
	planes[BOTTOM] = Plane(near_bottom_right, far_bottom_left, far_bottom_right);
	planes[NEAR]   = Plane(near_top_right, near_top_left, near_bottom_left);

	points[NEAR_TOP_LEFT]     = near_top_left;
	points[NEAR_TOP_RIGHT]    = near_top_right;
	points[NEAR_BOTTOM_LEFT]  = near_bottom_left;
	points[NEAR_BOTTOM_RIGHT] = near_bottom_right;
	points[FAR_TOP_LEFT]      = far_top_left;
	points[FAR_TOP_RIGHT]     = far_top_right;
	points[FAR_BOTTOM_LEFT]   = far_bottom_left;
	points[FAR_BOTTOM_RIGHT]  = far_bottom_right;
}

void Frustum::draw_debug() const noexcept
{
	const zcm::vec3 color{1.0f, 1.0f, 1.0f};
	const zcm::vec3 normcolor{1.0f, 0.0f, 0.0f};

	dd::line(points[NEAR_TOP_LEFT],     points[NEAR_TOP_RIGHT],   color);
	dd::line(points[NEAR_TOP_RIGHT],    points[NEAR_BOTTOM_RIGHT],color);
	dd::line(points[NEAR_BOTTOM_RIGHT], points[NEAR_BOTTOM_LEFT], color);
	dd::line(points[NEAR_BOTTOM_LEFT],  points[NEAR_TOP_LEFT],    color);

	dd::line(points[FAR_TOP_LEFT],      points[FAR_TOP_RIGHT],    color);
	dd::line(points[FAR_TOP_RIGHT],     points[FAR_BOTTOM_RIGHT], color);
	dd::line(points[FAR_BOTTOM_RIGHT],  points[FAR_BOTTOM_LEFT],  color);
	dd::line(points[FAR_BOTTOM_LEFT],   points[FAR_TOP_LEFT],     color);

	dd::line(points[NEAR_TOP_LEFT],     points[FAR_TOP_LEFT],     color);
	dd::line(points[NEAR_TOP_RIGHT],    points[FAR_TOP_RIGHT],    color);
	dd::line(points[NEAR_BOTTOM_RIGHT], points[FAR_BOTTOM_RIGHT], color);
	dd::line(points[NEAR_BOTTOM_LEFT],  points[FAR_BOTTOM_LEFT],  color);

	dd::sphere(near_center, normcolor, 0.01f);
	dd::sphere(far_center, normcolor, 0.01f);
	dd::line(near_center, far_center, normcolor);
}

bool Frustum::sphere_culled(const zcm::vec3 & pos, float r) const noexcept
{
	for(int i = 0; i < 5; ++i) {
		if(planes[i].distance(pos) < -r)
			return true;
	}
	return false;
}

bool Frustum::bbox_culled(const bbox3 & box) const noexcept
{
	// check box outside/inside of frustum
	for(int i = 0; i < 5; i++) {
		int out = 0;
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.min().x, box.min().y, box.min().z, 1.0f) ) < 0.0f )?1:0);
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.max().x, box.min().y, box.min().z, 1.0f) ) < 0.0f )?1:0);
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.min().x, box.max().y, box.min().z, 1.0f) ) < 0.0f )?1:0);
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.max().x, box.max().y, box.min().z, 1.0f) ) < 0.0f )?1:0);
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.min().x, box.min().y, box.max().z, 1.0f) ) < 0.0f )?1:0);
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.max().x, box.min().y, box.max().z, 1.0f) ) < 0.0f )?1:0);
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.min().x, box.max().y, box.max().z, 1.0f) ) < 0.0f )?1:0);
		out += ((zcm::dot(planes[i].plane, zcm::vec4(box.max().x, box.max().y, box.max().z, 1.0f) ) < 0.0f )?1:0);
		if(out == 8) return true;
	}

	// check frustum outside/inside box
	// requires far plane for now
	//		int out;
	//		out=0; for(int i = 0; i < 8; i++) out += ((points[i].x > box.max().x)?1:0); if(out == 8) return true;
	//		out=0; for(int i = 0; i < 8; i++) out += ((points[i].x < box.min().x)?1:0); if(out == 8) return true;
	//		out=0; for(int i = 0; i < 8; i++) out += ((points[i].y > box.max().y)?1:0); if(out == 8) return true;
	//		out=0; for(int i = 0; i < 8; i++) out += ((points[i].y < box.min().y)?1:0); if(out == 8) return true;
	//		out=0; for(int i = 0; i < 8; i++) out += ((points[i].z > box.max().z)?1:0); if(out == 8) return true;
	//		out=0; for(int i = 0; i < 8; i++) out += ((points[i].z < box.min().z)?1:0); if(out == 8) return true;

	return false;
}

ShadowFrustum::ShadowFrustum(const zcm::vec3 & pos, const zcm::vec3 & forward, const zcm::vec3 & up, float near, float radius)
{
	update(pos, forward, up, zcm::cross(forward, up), zcm::half_pi(), 1.0f, near, radius);
}
