#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/bbox.hpp>

namespace rc {

struct Plane
{
	glm::vec4 plane;

	Plane() = default;
	Plane(const glm::vec3 &_normal, const glm::vec3 &_point) noexcept;
	Plane(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) noexcept;
	float distance(const glm::vec3 & p) const noexcept;
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
	glm::vec3 points[8];
	glm::vec3 near_center, far_center;

	uint8_t state = NoState;

	void update(const glm::vec3& pos,
	            const glm::vec3& forward,
	            const glm::vec3& cup,
	            float yfov,
	            float aspect,
	            float near,
	            float far) noexcept;

	void draw_debug() const noexcept;

	bool sphere_culled(const glm::vec3& pos, float r) const noexcept;
	bool bbox_culled(const bbox3& box) const noexcept;
};

}
