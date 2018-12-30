#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/bbox.hpp>
#include <zcm/vec4.hpp>
#include <zcm/vec3.hpp>

namespace rc {

struct Plane
{
	zcm::vec4 plane;

	Plane() = default;
	Plane(const zcm::vec3 &_normal, const zcm::vec3 &_point) noexcept;
	Plane(const zcm::vec3 &a, const zcm::vec3 &b, const zcm::vec3 &c) noexcept;
	float distance(const zcm::vec3 & p) const noexcept;
};

struct Frustum
{
	enum State
	{
		NoState,
		Locked = 0x1,
		ShowWireframe = 0x2
	};

	Plane planes[5];
	zcm::vec3 points[8];
	zcm::vec3 near_center, far_center;

	unsigned char state = NoState;

	void update(const zcm::vec3& pos,
	            const zcm::vec3& forward,
	            const zcm::vec3& cup,
	            float yfov,
	            float aspect,
	            float near,
	            float far) noexcept;

	void draw_debug() const noexcept;

	bool sphere_culled(const zcm::vec3& pos, float r) const noexcept;
	bool bbox_culled(const bbox3& box) const noexcept;
};

}
