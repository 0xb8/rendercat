#include <cstring>
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
		return zcm::vec2(0.0f);
	}
}

zcm::vec3 bbox3::center() const noexcept
{
	if (!is_null()) {
		zcm::vec3 d = diagonal();
		return mMin + (d * float(0.5));
	} else {
		return zcm::vec3(0.0f);
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

zcm::vec3 bbox2::bounding_circle() const noexcept
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

zcm::vec4 bbox3::bounding_sphere() const noexcept
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


// -----------------------------------------------------------------------------
#include <doctest/doctest.h>
#include <zcm/bvec2.hpp>
#include <zcm/bvec3.hpp>
#include <cmath>

TEST_CASE("BBox construction") {

	rc::bbox2 bbox2;
	rc::bbox3 bbox3;

	SUBCASE("Default constructed bbox is null") {
		REQUIRE(bbox2.is_null());
		REQUIRE(bbox3.is_null());
	}

	SUBCASE("Default constructed bbox's center and diagnonal are zero-vectors") {
		REQUIRE(bbox2.center() == zcm::vec2(0.0f));
		REQUIRE(bbox3.center() == zcm::vec3(0.0f));

		REQUIRE(bbox2.diagonal() == zcm::vec2(0.0f));
		REQUIRE(bbox3.diagonal() == zcm::vec3(0.0f));
	}

	SUBCASE("Default constructed bbox does not intersect with anything, even itself") {
		REQUIRE(rc::bbox2::intersects_sphere(bbox2, zcm::vec2(0.0f), 10.0f) == rc::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects_sphere(bbox3, zcm::vec3(0.0f), 10.0f) == rc::Intersection::Outside);

		REQUIRE(rc::bbox2::intersects(bbox2, bbox2) == rc::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects(bbox3, bbox3) == rc::Intersection::Outside);

		// includes a circle/sphere at origin with radius 10.0
		rc::bbox2 bbox2a;
		bbox2a.include_circle(zcm::vec2(0.0f), 10.0f);
		rc::bbox3 bbox3a;
		bbox3a.include_sphere(zcm::vec3(0.0f), 10.0f);

		REQUIRE(rc::bbox2::intersects(bbox2, bbox2a) == rc::Intersection::Outside);
		REQUIRE(rc::bbox3::intersects(bbox3, bbox3a) == rc::Intersection::Outside);
	}
}

TEST_CASE("BBox include") {

	rc::bbox2 bbox2;
	rc::bbox3 bbox3;

	SUBCASE("single point in null bbox") {
		REQUIRE(bbox2.is_null());

		const auto point2 = zcm::vec2(3.0f, -2.0f);
		const auto point3 = zcm::vec3(3.0f, -2.0f, 1.0f);

		bbox2.include(point2);
		bbox3.include(point3);

		SUBCASE("Bbox with a point included is not null") {
			REQUIRE(!bbox2.is_null());
			REQUIRE(!bbox3.is_null());
		}

		SUBCASE("Bbox with a single point has min and max equal to that point") {
			REQUIRE(bbox2.min() == point2);
			REQUIRE(bbox2.max() == point2);
			REQUIRE(bbox3.min() == point3);
			REQUIRE(bbox3.max() == point3);
		}

		SUBCASE("Bbox with a single point has ranges equal to that point") {
			REQUIRE(bbox2.x_range().min == point2.x);
			REQUIRE(bbox2.x_range().max == point2.x);
			REQUIRE(bbox2.y_range().min == point2.y);
			REQUIRE(bbox2.y_range().max == point2.y);
			REQUIRE(bbox3.x_range().min == point3.x);
			REQUIRE(bbox3.x_range().max == point3.x);
			REQUIRE(bbox3.y_range().min == point3.y);
			REQUIRE(bbox3.y_range().max == point3.y);
			REQUIRE(bbox3.z_range().min == point3.z);
			REQUIRE(bbox3.z_range().max == point3.z);
		}

		SUBCASE("Bbox with a single point has zero diagonal") {
			REQUIRE(bbox2.diagonal() == zcm::vec2(0.0f));
			REQUIRE(bbox3.diagonal() == zcm::vec3(0.0f));
		}

		SUBCASE("Bbox with a single point has center in that point") {
			REQUIRE(bbox2.center() == point2);
			REQUIRE(bbox3.center() == point3);
		}

		SUBCASE("Closest point on bbox with a single point is that point") {
			REQUIRE(bbox2.closest_point(zcm::vec2(123.4f, 567.8f)) == point2);
			REQUIRE(bbox3.closest_point(zcm::vec3(123.4f, 567.8f, 910.11f)) == point3);
		}
	}

	SUBCASE("multiple points") {

		rc::bbox2 bbox2;
		rc::bbox3 bbox3;

		zcm::vec2 a2(0.0f), b2(1.0f), c2(-1.0f, 10.0f), d2(0.5f, -0.5f), e2(-3.0f, 3.0f);
		zcm::vec3 a3(0.0f), b3(-1.0f, -2.0f, -3.0f), c3(5.0f, 3.0f, 4.0f), d3(0.1f, -1.0f, 2.0f), e3(-24.0f, 0.0f, 1.045f);

		SUBCASE("Normal include") {
			bbox2.include(a2);
			bbox2.include(b2);
			bbox2.include(c2);
			bbox2.include(d2);
			bbox2.include(e2);

			bbox3.include(a3);
			bbox3.include(b3);
			bbox3.include(c3);
			bbox3.include(d3);
			bbox3.include(e3);

			REQUIRE(!bbox2.is_null());
			REQUIRE(bbox2.min() == zcm::vec2(-3.0f, -0.5f));
			REQUIRE(bbox2.max() == zcm::vec2(1.0f, 10.0f));

			REQUIRE(!bbox3.is_null());
			REQUIRE(bbox3.min() == zcm::vec3(-24.0f, -2.0f, -3.0f));
			REQUIRE(bbox3.max() == zcm::vec3(5.0f, 3.0f, 4.0f));

		}
	}

	SUBCASE("Include NaN") {

		rc::bbox2 bbox2;
		rc::bbox3 bbox3;

		SUBCASE("Null bbox should stay null") {
			bbox2.include(zcm::vec2(NAN));
			bbox3.include(zcm::vec3(NAN));

			REQUIRE(bbox2.is_null());
			REQUIRE(bbox3.is_null());
			REQUIRE(!zcm::any(zcm::isnan(bbox2.min())));
			REQUIRE(!zcm::any(zcm::isnan(bbox2.max())));
			REQUIRE(!zcm::any(zcm::isnan(bbox3.min())));
			REQUIRE(!zcm::any(zcm::isnan(bbox3.max())));
		}

		zcm::vec2 a2(0.0f), b2(1.0f), c2(-1.0f, 10.0f), d2(0.5f, -0.5f), e2(-3.0f, 3.0f);
		zcm::vec3 a3(0.0f), b3(-1.0f, -2.0f, -3.0f), c3(5.0f, 3.0f, 4.0f), d3(0.1f, -1.0f, 2.0f), e3(-24.0f, 0.0f, 1.045f);
		bbox2.include(a2);
		bbox2.include(b2);
		bbox2.include(c2);
		bbox2.include(d2);
		bbox2.include(e2);
		bbox3.include(a3);
		bbox3.include(b3);
		bbox3.include(c3);
		bbox3.include(d3);
		bbox3.include(e3);

		auto bbox2_nan = bbox2;
		auto bbox3_nan = bbox3;


		SUBCASE("Single point") {
			bbox2_nan.include(zcm::vec2(NAN, 1.1f));
			bbox2_nan.include(zcm::vec2(0.1f, NAN));
			bbox2_nan.include(zcm::vec2(0.1f, std::nanf("1234")));
			bbox2_nan.include(zcm::vec2(std::nanf("5678"), 0.1f));

			bbox3_nan.include(zcm::vec3(NAN, 1.1f, 0.0f));
			bbox3_nan.include(zcm::vec3(0.1f, NAN, 0.1f));
			bbox3_nan.include(zcm::vec3(0.1f, 0.1f, NAN));
			bbox3_nan.include(zcm::vec3(0.1f, std::nanf("1234"), NAN));
			bbox3_nan.include(zcm::vec3(std::nanf("5678"), 0.1f, NAN));

			REQUIRE(!bbox2_nan.is_null());
			REQUIRE(!zcm::any(zcm::isnan(bbox2_nan.min())));
			REQUIRE(!zcm::any(zcm::isnan(bbox2_nan.max())));
			REQUIRE(bbox2_nan.min() == bbox2.min());
			REQUIRE(bbox2_nan.max() == bbox2.max());

			REQUIRE(!bbox3_nan.is_null());
			REQUIRE(!zcm::any(zcm::isnan(bbox3_nan.min())));
			REQUIRE(!zcm::any(zcm::isnan(bbox3_nan.max())));
			REQUIRE(bbox3_nan.min() == bbox3.min());
			REQUIRE(bbox3_nan.max() == bbox3.max());
		}

		SUBCASE("Bbox") {
			// we can't actually create NaN bbox, so instead we use fake one
			struct NanBbox2
			{
				zcm::vec2 min;
				zcm::vec2 max;
			};

			struct NanBbox3
			{
				zcm::vec3 min;
				zcm::vec3 max;
			};

			REQUIRE(sizeof(NanBbox2) == sizeof(rc::bbox2));
			REQUIRE(sizeof(NanBbox3) == sizeof(rc::bbox3));

			auto make_nanbox2 = [](NanBbox2 src) {
				rc::bbox2 res;
				memcpy(&res, &src, sizeof(res));
				return res;
			};

			auto make_nanbox3 = [](NanBbox3 src) {
				rc::bbox3 res;
				memcpy(&res, &src, sizeof(res));
				return res;
			};

			NanBbox2 fakebbox2{zcm::vec2(NAN, 0.0f), zcm::vec2(0.0f, NAN)};
			NanBbox2 fakebbox22{zcm::vec2(0.0f, NAN), zcm::vec2(NAN, 0.0f)};
			NanBbox2 fakebbox23{zcm::vec2(NAN), zcm::vec2(NAN)};
			REQUIRE(zcm::any(zcm::isnan(fakebbox2.min)));
			REQUIRE(zcm::any(zcm::isnan(fakebbox2.max)));
			REQUIRE(zcm::any(zcm::isnan(fakebbox22.min)));
			REQUIRE(zcm::any(zcm::isnan(fakebbox22.max)));
			REQUIRE(zcm::all(zcm::isnan(fakebbox23.min)));
			REQUIRE(zcm::all(zcm::isnan(fakebbox23.max)));

			NanBbox3 fakebbox3{zcm::vec3(NAN, 0.0f, NAN), zcm::vec3(0.0f, NAN, 0.0f)};
			NanBbox3 fakebbox32{zcm::vec3(0.0f, NAN, NAN), zcm::vec3(NAN, NAN, 0.0f)};
			NanBbox3 fakebbox33{zcm::vec3(NAN, 0.0f, 0.0), zcm::vec3(NAN, NAN, NAN)};
			REQUIRE(zcm::any(zcm::isnan(fakebbox3.min)));
			REQUIRE(zcm::any(zcm::isnan(fakebbox3.max)));
			REQUIRE(zcm::any(zcm::isnan(fakebbox32.min)));
			REQUIRE(zcm::any(zcm::isnan(fakebbox32.max)));
			REQUIRE(zcm::any(zcm::isnan(fakebbox33.min)));
			REQUIRE(zcm::all(zcm::isnan(fakebbox33.max)));

			bbox2_nan.include(make_nanbox2(fakebbox2));
			bbox2_nan.include(make_nanbox2(fakebbox22));
			bbox2_nan.include(make_nanbox2(fakebbox23));

			bbox3_nan.include(make_nanbox3(fakebbox3));
			bbox3_nan.include(make_nanbox3(fakebbox32));
			bbox3_nan.include(make_nanbox3(fakebbox33));

			SUBCASE("Nan bbox should become null") {
				REQUIRE(make_nanbox2(fakebbox2).is_null());
				REQUIRE(make_nanbox2(fakebbox22).is_null());
				REQUIRE(make_nanbox2(fakebbox23).is_null());

				REQUIRE(make_nanbox3(fakebbox3).is_null());
				REQUIRE(make_nanbox3(fakebbox32).is_null());
				REQUIRE(make_nanbox3(fakebbox33).is_null());

				SUBCASE("And stay null") {
					auto bbox2 = make_nanbox2(fakebbox2);
					bbox2.include(zcm::vec2(1213.f, 32242.f));
					bbox2.include(zcm::vec2(-223.f,-54.32f));

					auto bbox3 = make_nanbox3(fakebbox3);
					bbox3.include(zcm::vec3(1213.f, 32242.f, 12312.f));
					bbox3.include(zcm::vec3(-223.f,-54.32f, -232.f));

					REQUIRE(bbox2.is_null());
					REQUIRE(bbox3.is_null());
				}
			}

			REQUIRE(!bbox2_nan.is_null());
			REQUIRE(!zcm::any(zcm::isnan(bbox2.min())));
			REQUIRE(!zcm::any(zcm::isnan(bbox2.max())));
			REQUIRE(bbox2_nan.min() == bbox2.min());
			REQUIRE(bbox2_nan.max() == bbox2.max());

			REQUIRE(!bbox3_nan.is_null());
			REQUIRE(!zcm::any(zcm::isnan(bbox3.min())));
			REQUIRE(!zcm::any(zcm::isnan(bbox3.max())));
			REQUIRE(bbox3_nan.min() == bbox3.min());
			REQUIRE(bbox3_nan.max() == bbox3.max());

		}

		SUBCASE("Circle") {

			bbox2_nan.include_circle(zcm::vec2(12.0f), NAN);
			bbox2_nan.include_circle(zcm::vec2(43.0f, NAN), NAN);
			bbox2_nan.include_circle(zcm::vec2(NAN, 1.0f), NAN);
			bbox2_nan.include_circle(zcm::vec2(NAN), NAN);

			REQUIRE(!bbox2_nan.is_null());
			REQUIRE(!zcm::any(zcm::isnan(bbox2.min())));
			REQUIRE(!zcm::any(zcm::isnan(bbox2.max())));
			REQUIRE(bbox2_nan.min() == bbox2.min());
			REQUIRE(bbox2_nan.max() == bbox2.max());
		}
	}
}

TEST_CASE("BBox extend") {

		rc::bbox2 bbox2;
		bbox2.include(zcm::vec2(10.0f, -10.0f));
		bbox2.include(zcm::vec2(0.0f, 10.0f));

		rc::bbox3 bbox3;
		bbox3.include(zcm::vec3(10.0f, -10.0f, 0.0f));
		bbox3.include(zcm::vec3(0.0f, 10.0f, -10.0f));

		rc::bbox2 bbox2_ex = bbox2;
		rc::bbox3 bbox3_ex = bbox3;

		SUBCASE("Zero") {

			bbox2_ex.extend(0.0f);
			bbox3_ex.extend(0.0f);

			REQUIRE(bbox2_ex.min() == bbox2.min());
			REQUIRE(bbox2_ex.max() == bbox2.max());

			REQUIRE(bbox3_ex.min() == bbox3.min());
			REQUIRE(bbox3_ex.max() == bbox3.max());
		}

		SUBCASE("Positive") {
			bbox2_ex.extend(10.0f);
			bbox3_ex.extend(10.0f);

			REQUIRE(bbox2_ex.min() == zcm::vec2(-10.0f, -20.0f));
			REQUIRE(bbox2_ex.max() == zcm::vec2(20.0f, 20.0f));

			REQUIRE(bbox3_ex.min() == zcm::vec3(-10.0f, -20.0f, -20.0f));
			REQUIRE(bbox3_ex.max() == zcm::vec3(20.0f, 20.0f, 10.0f));
		}

		SUBCASE("Negative") {
			bbox2_ex.extend(-5.0f);
			bbox3_ex.extend(-5.0f);

			REQUIRE(bbox2_ex.min() == zcm::vec2(5.0f, -5.0f));
			REQUIRE(bbox2_ex.max() == zcm::vec2(5.0f, 5.0f));

			REQUIRE(bbox3_ex.min() == zcm::vec3(5.0f, -5.0f, -5.0f));
			REQUIRE(bbox3_ex.max() == zcm::vec3(5.0f, 5.0f, -5.0f));
		}
}


