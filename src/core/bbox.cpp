#include <rendercat/core/bbox.hpp>
#include <zcm/component_wise.hpp>
#include <zcm/geometric.hpp>
#include <zcm/mat3.hpp>
#include <zcm/mat4.hpp>
#include <zcm/quat.hpp>
#include <zcm/common.hpp>
#include <zcm/angle_and_trigonometry.hpp>
#include <zcm/exponential.hpp>
#include <cassert>

using namespace rc;

bbox2 bbox2::transformed(const bbox2& bbox, const zcm::mat3& m) noexcept
{
	const zcm::vec2 points[4] {
		{bbox.mMin.x, bbox.mMin.y},
		{bbox.mMin.x, bbox.mMax.y},
		{bbox.mMax.x, bbox.mMin.y},
		{bbox.mMax.x, bbox.mMax.y}
	};

	bbox2 res;
	for(int i = 0; i < 4; ++i) {
		auto p{m * zcm::vec3(points[i], 1.0f)};
		res.include(zcm::vec2{p.x, p.y});
	}
	return res;
}

bbox2::bbox2(zcm::vec2 p1, zcm::vec2 p2) noexcept
{
	include(p1);
	include(p2);
}

bool bbox2::is_null() const noexcept {
	return !(mMin.x <= mMax.x && mMin.y <= mMax.y);
}

void bbox2::include(zcm::vec2 p) noexcept
{
	mMin = zcm::min(mMin, p);
	mMax = zcm::max(mMax, p);
}

void bbox2::include(const bbox2 & bbox) noexcept
{
	mMin = zcm::min(mMin, bbox.mMin);
	mMax = zcm::max(mMax, bbox.mMax);
}

void bbox2::include_circle(zcm::vec2 p, float radius) noexcept
{
	zcm::vec2 r(radius);
	mMin = zcm::min(mMin, p - r);
	mMax = zcm::max(mMax, p + r);
}

void bbox2::extend(float val) noexcept
{
	assert(!is_null());
	assert(!zcm::isnan(val));
	mMin -= zcm::vec2(val);
	mMax += zcm::vec2(val);
}

void bbox2::translate(zcm::vec2 v) noexcept
{
	assert(!is_null()); // TODO: nan-safety
	mMin += v;
	mMax += v;
}

void bbox2::scale(zcm::vec2 scale, zcm::vec2 origin) noexcept
{
	assert(!is_null());
	mMin -= origin;
	mMax -= origin;

	mMin *= scale;
	mMax *= scale;

	mMin += origin;
	mMax += origin;
}

void bbox2::scale(const zcm::vec2 & scale_) noexcept
{
	scale(scale_, center());
}

void bbox3::scale(zcm::vec3 scale, zcm::vec3 origin) noexcept
{
	assert(!is_null());
	mMin -= origin;
	mMax -= origin;

	mMin *= scale;
	mMax *= scale;

	mMin += origin;
	mMax += origin;
}

void bbox3::scale(zcm::vec3 scale_) noexcept
{
	scale(scale_, center());
}

zcm::vec2 bbox2::center() const noexcept
{
	if (!is_null()) {
		zcm::vec2 d = diagonal();
		return mMin + (d * float(0.5));
	} else {
		return zcm::vec2(0.0);
	}
}

zcm::vec3 bbox3::center() const noexcept
{
	if (!is_null()) {
		zcm::vec3 d = diagonal();
		return mMin + (d * float(0.5));
	} else {
		return zcm::vec3(0.0);
	}
}

zcm::vec2 bbox2::diagonal() const noexcept
{
	if (!is_null())
		return mMax - mMin;
	else
		return zcm::vec2(0);
}

zcm::vec3 bbox3::diagonal() const noexcept
{
	if (!is_null())
		return mMax - mMin;
	else
		return zcm::vec3(0);
}

float bbox2::longest_edge() const noexcept
{
	return zcm::compMax(diagonal());
}

float bbox3::longest_edge() const noexcept
{
	return zcm::compMax(diagonal());
}

float bbox2::shortest_edge() const noexcept
{
	return zcm::compMin(diagonal());
}

zcm::vec2 bbox2::min() const noexcept
{
	return mMin;
}

zcm::vec2 bbox2::max() const noexcept
{
	return mMax;
}

bbox2::range bbox2::x_range() const noexcept
{
	return range{mMin.x, mMax.x};
}

bbox2::range bbox2::y_range() const noexcept
{
	return range{mMin.y, mMax.y};
}

zcm::vec2 bbox2::closest_point(zcm::vec2 point) const noexcept
{
	return zcm::clamp(point, mMin, mMax);
}

glm::vec3 bbox2::bounding_circle() const noexcept
{
	auto rad = zcm::length(diagonal()) * 0.5f;
	return zcm::vec3{center(), rad};
}

bbox3::bbox3(zcm::vec3 p1, zcm::vec3 p2) noexcept
{
	include(p1);
	include(p2);
}

bool bbox3::is_null() const noexcept
{
	return !(mMin.x <= mMax.x && mMin.y <= mMax.y && mMin.z <= mMax.z);
}

void bbox3::extend(float val) noexcept
{
	assert(!is_null());
	assert(!zcm::isnan(val));
	mMin -= zcm::vec3(val);
	mMax += zcm::vec3(val);

}

void bbox3::include(zcm::vec3 p) noexcept
{
	mMin = zcm::min(mMin, p);
	mMax = zcm::max(mMax, p);
}

void bbox3::include(const bbox3 & bbox) noexcept
{
	mMin = zcm::min(mMin, bbox.mMin);
	mMax = zcm::max(mMax, bbox.mMax);
}

void bbox3::include_sphere(zcm::vec3 p, float radius) noexcept
{
	zcm::vec3 r(radius);
	mMin = zcm::min(mMin, p - r);
	mMax = zcm::max(mMax, p + r);
}

void bbox3::translate(zcm::vec3 v) noexcept
{
	assert(!is_null());
	mMin += v;
	mMax += v;
}

bbox3 bbox3::transformed(const bbox3& bbox, const zcm::mat4 &m) noexcept
{
	const zcm::vec3 points[8] {
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
		auto p {m * zcm::vec4(points[i], 1.0f)};
		res.include({p.x, p.y, p.z});
	}
	return res;
}

bbox3 bbox3::rotated(const bbox3 & bbox, const zcm::quat & quat) noexcept
{
	bbox3 res;
	res.include(quat * zcm::vec3{bbox.mMin.x, bbox.mMin.y, bbox.mMin.z});
	res.include(quat * zcm::vec3{bbox.mMin.x, bbox.mMin.y, bbox.mMax.z});
	res.include(quat * zcm::vec3{bbox.mMin.x, bbox.mMax.y, bbox.mMin.z});
	res.include(quat * zcm::vec3{bbox.mMin.x, bbox.mMax.y, bbox.mMax.z});
	res.include(quat * zcm::vec3{bbox.mMax.x, bbox.mMin.y, bbox.mMin.z});
	res.include(quat * zcm::vec3{bbox.mMax.x, bbox.mMin.y, bbox.mMax.z});
	res.include(quat * zcm::vec3{bbox.mMax.x, bbox.mMax.y, bbox.mMin.z});
	res.include(quat * zcm::vec3{bbox.mMax.x, bbox.mMax.y, bbox.mMax.z});
	return res;
}

float bbox3::shortest_edge() const noexcept
{
	return zcm::compMin(diagonal());
}

zcm::vec3 bbox3::min() const noexcept
{
	return mMin;
}

zcm::vec3 bbox3::max() const noexcept
{
	return mMax;
}

bbox3::range bbox3::x_range() const noexcept
{
	return range{mMin.x, mMax.x};
}

bbox3::range bbox3::y_range() const noexcept
{
	return range{mMin.y, mMax.y};
}

bbox3::range bbox3::z_range() const noexcept
{
	return range{mMin.z, mMax.z};
}

zcm::vec3 bbox3::closest_point(zcm::vec3 point) const noexcept
{
	return zcm::clamp(point, mMin, mMax);
}

glm::vec4 bbox3::bounding_sphere() const noexcept
{
	auto rad = zcm::length(diagonal()) * 0.5f;
	return zcm::vec4{center(), rad};
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

static Intersection intersects_sphere_impl(zcm::vec3 closest_to_center,
                       zcm::vec3 center,
                       float radius) noexcept
{
	double distance2 = zcm::length2(center - closest_to_center);
	auto radius2 = radius*radius;
	return static_cast<Intersection>(distance2 <= radius2);
}

Intersection bbox2::intersects_sphere(const bbox2& bbox, const zcm::vec2& center, float radius) noexcept
{
	if(bbox.is_null()) return Intersection::Outside;
	return intersects_sphere_impl(zcm::vec3(bbox.closest_point(center), 0.0f),
	                              zcm::vec3(center, 0.0f),
	                              radius);
}

Intersection bbox3::intersects_sphere(const bbox3& bbox, zcm::vec3 center, float radius) noexcept
{
	if(bbox.is_null()) return Intersection::Outside;
	return intersects_sphere_impl(bbox.closest_point(center),
	                              center,
	                              radius);
}

Intersection bbox3::intersects_cone(const bbox3 & bbox, zcm::vec3 origin, zcm::vec3 forward, float angle, float size) noexcept
{
	auto sphere = bbox.bounding_sphere();

	// ref: https://bartwronski.com/2017/04/13/cull-that-cone/
	// TODO: separate into sphere/cone test
	auto V = sphere.xyz - origin;
	float  VlenSq = zcm::length2(V);
	float  V1len  = zcm::dot(V, forward);
	float  distanceClosestPoint = zcm::cos(angle) * zcm::sqrt(VlenSq - V1len*V1len) - V1len * zcm::sin(angle);

	bool angleCull = distanceClosestPoint > sphere.w;
	bool frontCull = V1len >  sphere.w + size;
	bool backCull  = V1len < -sphere.w;
	return (angleCull || frontCull || backCull) ? Intersection::Outside : Intersection::Intersect;
}

Intersection bbox2::intersects_ray(const bbox2& bbox, const ray2_inv& ray) noexcept
{
	auto t1 = (bbox.mMin[0] - ray.origin[0])*ray.dir_inv[0];
	auto t2 = (bbox.mMax[0] - ray.origin[0])*ray.dir_inv[0];

	auto tmin = zcm::min(t1, t2);
	auto tmax = zcm::max(t1, t2);

	for (int i = 1; i < 2; ++i) {
		t1 = (bbox.mMin[i] - ray.origin[i])*ray.dir_inv[i];
		t2 = (bbox.mMax[i] - ray.origin[i])*ray.dir_inv[i];

		tmin = zcm::max(tmin, zcm::min(zcm::min(t1, t2), tmax));
		tmax = zcm::min(tmax, zcm::max(zcm::max(t1, t2), tmin));
	}

	return static_cast<Intersection>(tmax > zcm::max(tmin, 0.0f));
}

Intersection bbox3::intersects_ray(const bbox3& bbox, const ray3_inv& ray) noexcept
{
	auto t1 = (bbox.mMin[0] - ray.origin[0])*ray.dir_inv[0];
	auto t2 = (bbox.mMax[0] - ray.origin[0])*ray.dir_inv[0];

	auto tmin = zcm::min(t1, t2);
	auto tmax = zcm::max(t1, t2);

	for (int i = 1; i < 3; ++i) {
		t1 = (bbox.mMin[i] - ray.origin[i])*ray.dir_inv[i];
		t2 = (bbox.mMax[i] - ray.origin[i])*ray.dir_inv[i];

		tmin = zcm::max(tmin, zcm::min(zcm::min(t1, t2), tmax));
		tmax = zcm::min(tmax, zcm::max(zcm::max(t1, t2), tmin));
	}

	return static_cast<Intersection>(tmax > zcm::max(tmin, 0.0f));
}

