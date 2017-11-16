#include <rendercat/core/AABB.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/mat4x4.hpp>

using namespace rc;

AABB AABB::transformed(const glm::mat4 &m) const noexcept
{
	const glm::vec3 points[8] {
		{mMin.x, mMin.y, mMin.z},
		{mMin.x, mMin.y, mMax.z},
		{mMin.x, mMax.y, mMin.z},
		{mMin.x, mMax.y, mMax.z},
		{mMax.x, mMin.y, mMin.z},
		{mMax.x, mMin.y, mMax.z},
		{mMax.x, mMax.y, mMin.z},
		{mMax.x, mMax.y, mMax.z}
	};

	AABB res;
	for(int i = 0; i < 8; ++i) {
		res.include(m * glm::vec4(points[i], 1.0f));
	}
	return res;
}

void AABB::scale(const glm::vec3 & scale, const glm::vec3 & origin) noexcept
{
	assert(!is_null());
	mMin -= origin;
	mMax -= origin;

	mMin *= scale;
	mMax *= scale;

	mMin += origin;
	mMax += origin;

}

glm::vec3 AABB::center() const noexcept
{
	if (!is_null()) {
		glm::vec3 d = diagonal();
		return mMin + (d * glm::float_t(0.5));
	} else {
		return glm::vec3(0.0);
	}
}

glm::vec3 AABB::diagonal() const noexcept
{
	if (!is_null())
		return mMax - mMin;
	else
		return glm::vec3(0);
}

glm::float_t AABB::longest_edge() const noexcept
{
	return glm::compMax(diagonal());
}

glm::float_t AABB::shortest_edge() const noexcept
{
	return glm::compMin(diagonal());
}

AABB::Intersection AABB::intersects(const AABB & b) const noexcept
{
	if (is_null() || b.is_null())
		return Intersection::Outside;

	if ((mMax.x < b.mMin.x) || (mMin.x > b.mMax.x) ||
	    (mMax.y < b.mMin.y) || (mMin.y > b.mMax.y) ||
	    (mMax.z < b.mMin.z) || (mMin.z > b.mMax.z))
	{
		return Intersection::Outside;
	}

	if ((mMin.x <= b.mMin.x) && (mMax.x >= b.mMax.x) &&
	    (mMin.y <= b.mMin.y) && (mMax.y >= b.mMax.y) &&
	    (mMin.z <= b.mMin.z) && (mMax.z >= b.mMax.z))
	{
		return Intersection::Inside;
	}

	return Intersection::Intersect;
}

bool AABB::intersects_sphere(const glm::vec3 & pos, float r) const noexcept
{
	double distance2 = glm::length2(pos - closest_point(pos));
	return distance2 < r * r;
}


