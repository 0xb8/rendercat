#include <rendercat/core/bbox.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/mat4x4.hpp>

using namespace rc;

bbox2 bbox2::transformed(const bbox2& bbox, const glm::mat3& m) noexcept
{
	const glm::vec2 points[4] {
		{bbox.mMin.x, bbox.mMin.y},
		{bbox.mMin.x, bbox.mMax.y},
		{bbox.mMax.x, bbox.mMin.y},
		{bbox.mMax.x, bbox.mMax.y}
	};

	bbox2 res;
	for(int i = 0; i < 4; ++i) {
		glm::vec2 p(m * glm::vec3(points[i], 1.0f));
		res.include(p);
	}
	return res;
}

bbox3 bbox3::transformed(const bbox3& bbox, const glm::mat4 &m) noexcept
{
	const glm::vec3 points[8] {
		{bbox.mMin.x, bbox.mMin.y, bbox.mMin.z},
		{bbox.mMin.x, bbox.mMin.y, bbox.mMax.z},
		{bbox.mMin.x, bbox.mMax.y, bbox.mMin.z},
		{bbox.mMin.x, bbox.mMax.y, bbox.mMax.z},
		{bbox.mMax.x, bbox.mMin.y, bbox.mMin.z},
		{bbox.mMax.x, bbox.mMin.y, bbox.mMax.z},
		{bbox.mMax.x, bbox.mMax.y, bbox.mMin.z},
		{bbox.mMax.x, bbox.mMax.y, bbox.mMax.z}
	};

	bbox3 res;
	for(int i = 0; i < 8; ++i) {
		glm::vec3 p(m * glm::vec4(points[i], 1.0f));
		res.include(p);
	}
	return res;
}

void bbox2::scale(const glm::vec2& scale, const glm::vec2& origin) noexcept
{
	assert(!is_null());
	mMin -= origin;
	mMax -= origin;

	mMin *= scale;
	mMax *= scale;

	mMin += origin;
	mMax += origin;
}

void bbox3::scale(const glm::vec3 & scale, const glm::vec3 & origin) noexcept
{
	assert(!is_null());
	mMin -= origin;
	mMax -= origin;

	mMin *= scale;
	mMax *= scale;

	mMin += origin;
	mMax += origin;
}

glm::vec2 bbox2::center() const noexcept
{
	if (!is_null()) {
		glm::vec2 d = diagonal();
		return mMin + (d * glm::float_t(0.5));
	} else {
		return glm::vec2(0.0);
	}
}

glm::vec3 bbox3::center() const noexcept
{
	if (!is_null()) {
		glm::vec3 d = diagonal();
		return mMin + (d * glm::float_t(0.5));
	} else {
		return glm::vec3(0.0);
	}
}

glm::vec2 bbox2::diagonal() const noexcept
{
	if (!is_null())
		return mMax - mMin;
	else
		return glm::vec2(0);
}

glm::vec3 bbox3::diagonal() const noexcept
{
	if (!is_null())
		return mMax - mMin;
	else
		return glm::vec3(0);
}

glm::float_t bbox2::longest_edge() const noexcept
{
	return glm::compMax(diagonal());
}

glm::float_t bbox3::longest_edge() const noexcept
{
	return glm::compMax(diagonal());
}

glm::float_t bbox2::shortest_edge() const noexcept
{
	return glm::compMin(diagonal());
}

glm::float_t bbox3::shortest_edge() const noexcept
{
	return glm::compMin(diagonal());
}

Intersection bbox2::intersects(const bbox2 & a, const bbox2 & b) noexcept
{
	if (a.is_null() || b.is_null())
		return Intersection::Outside;

	if ((a.mMax.x < b.mMin.x) || (a.mMin.x > b.mMax.x) ||
	    (a.mMax.y < b.mMin.y) || (a.mMin.y > b.mMax.y))
	{
		return Intersection::Outside;
	}

	if ((a.mMin.x <= b.mMin.x) && (a.mMax.x >= b.mMax.x) &&
	    (a.mMin.y <= b.mMin.y) && (a.mMax.y >= b.mMax.y))
	{
		return Intersection::Inside;
	}

	return Intersection::Intersect;
}

Intersection bbox3::intersects(const bbox3& a, const bbox3& b) noexcept
{
	if (a.is_null() || b.is_null())
		return Intersection::Outside;

	if ((a.mMax.x < b.mMin.x) || (a.mMin.x > b.mMax.x) ||
	    (a.mMax.y < b.mMin.y) || (a.mMin.y > b.mMax.y) ||
	    (a.mMax.z < b.mMin.z) || (a.mMin.z > b.mMax.z))
	{
		return Intersection::Outside;
	}

	if ((a.mMin.x <= b.mMin.x) && (a.mMax.x >= b.mMax.x) &&
	    (a.mMin.y <= b.mMin.y) && (a.mMax.y >= b.mMax.y) &&
	    (a.mMin.z <= b.mMin.z) && (a.mMax.z >= b.mMax.z))
	{
		return Intersection::Inside;
	}

	return Intersection::Intersect;
}

static Intersection intersects_sphere_impl(glm::vec3 closest_to_center,
                       glm::vec3 center,
                       glm::float_t radius) noexcept
{
	double distance2 = glm::length2(center - closest_to_center);
	auto radius2 = radius*radius;
	return static_cast<Intersection>(distance2 <= radius2);
}

Intersection bbox2::intersects_sphere(const bbox2& bbox, const glm::vec2& center, glm::float_t radius) noexcept
{
	if(bbox.is_null()) return Intersection::Outside;
	return intersects_sphere_impl(glm::vec3(bbox.closest_point(center), 0.0f),
	                              glm::vec3(center, 0.0f),
	                              radius);
}

Intersection bbox3::intersects_sphere(const bbox3& bbox, const glm::vec3& center, glm::float_t radius) noexcept
{
	if(bbox.is_null()) return Intersection::Outside;
	return intersects_sphere_impl(bbox.closest_point(center),
	                              center,
	                              radius);
}

Intersection bbox2::intersects_ray(const bbox2& bbox, const ray2_inv& ray) noexcept
{
	auto t1 = (bbox.mMin[0] - ray.origin[0])*ray.dir_inv[0];
	auto t2 = (bbox.mMax[0] - ray.origin[0])*ray.dir_inv[0];

	auto tmin = glm::min(t1, t2);
	auto tmax = glm::max(t1, t2);

	for (int i = 1; i < 2; ++i) {
		t1 = (bbox.mMin[i] - ray.origin[i])*ray.dir_inv[i];
		t2 = (bbox.mMax[i] - ray.origin[i])*ray.dir_inv[i];

		tmin = glm::max(tmin, glm::min(glm::min(t1, t2), tmax));
		tmax = glm::min(tmax, glm::max(glm::max(t1, t2), tmin));
	}

	return static_cast<Intersection>(tmax > glm::max(tmin, 0.0f));
}

Intersection bbox3::intersects_ray(const bbox3& bbox, const ray3_inv& ray) noexcept
{
	auto t1 = (bbox.mMin[0] - ray.origin[0])*ray.dir_inv[0];
	auto t2 = (bbox.mMax[0] - ray.origin[0])*ray.dir_inv[0];

	auto tmin = glm::min(t1, t2);
	auto tmax = glm::max(t1, t2);

	for (int i = 1; i < 3; ++i) {
		t1 = (bbox.mMin[i] - ray.origin[i])*ray.dir_inv[i];
		t2 = (bbox.mMax[i] - ray.origin[i])*ray.dir_inv[i];

		tmin = glm::max(tmin, glm::min(glm::min(t1, t2), tmax));
		tmax = glm::min(tmax, glm::max(glm::max(t1, t2), tmin));
	}

	return static_cast<Intersection>(tmax > glm::max(tmin, 0.0f));
}

