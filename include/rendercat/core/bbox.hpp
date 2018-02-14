#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include <limits>

namespace rc {

struct _bbox_base
{
	/// Range of axis' coordinates.
	struct range
	{
		glm::float_t min;
		glm::float_t max;
	};

	/// Returns range from two values. If invalid, returns null range.
	static range _make_range(glm::float_t min, glm::float_t max) noexcept
	{
		if(min > max) return range{glm::float_t{}, glm::float_t{}};
		return range{min, max};
	}
};


class bbox2 : _bbox_base
{
public:

	bbox2() = default;

	/// Builds an bbox that contains the two points.
	bbox2(const glm::vec2& p1, glm::vec2& p2) noexcept
	{
		include(p1);
		include(p2);
	}

	/// Returns true if bbox is NULL (not set).
	bool is_null() const noexcept {return mMin.x > mMax.x || mMin.y > mMax.y;}

	/// Expand the bbox to include point \p p.
	void include(const glm::vec2& p) noexcept
	{
		mMin = glm::min(mMin, p);
		mMax = glm::max(mMax, p);
	}

	/// Extend the bounding box on all sides by \p val.
	void extend(glm::float_t val) noexcept
	{
		assert(!is_null());
		mMin -= glm::vec2(val);
		mMax += glm::vec2(val);

	}

	/// Translates bbox by vector \p v.
	void translate(const glm::vec2& v) noexcept
	{
		assert(!is_null());
		mMin += v;
		mMax += v;
	}

	/// Scale the bbox by \p scale, centered around \p origin.
	/// \param[in]  scale  2D vector specifying scale along each axis.
	/// \param[in]  origin Origin of scaling operation. Most useful origin would
	///                    be the center of the bbox.
	/// \see scale(const glm::vec2& scale)
	void scale(const glm::vec2& scale, const glm::vec2& origin) noexcept;

	/// Scale the bbox by \p scale, relative to the center of bbox.
	/// \param[in]  scale  2D vector specifying scale along each axis.
	void scale(const glm::vec2& scale_) noexcept
	{
		scale(scale_, center());
	}

	/// Returns transformed version of this bbox.
	/// \param[in]  mat  A transformation matrix.
	bbox2 transformed(const glm::mat3& mat) const noexcept;

	/// Retrieves the center of the bbox.
	glm::vec2 center() const noexcept;

	/// Retrieves the diagonal vector (computed as mMax - mMin).
	/// If the bbox is NULL, then a vector of all zeros is returned.
	glm::vec2 diagonal() const noexcept;

	/// Retrieves the longest edge.
	/// If the bbox is NULL, then 0 is returned.
	glm::float_t longest_edge() const noexcept;

	/// Retrieves the shortest edge.
	/// If the bbox is NULL, then 0 is returned.
	glm::float_t shortest_edge() const noexcept;

	/// Retrieves the bbox's minimum point.
	glm::vec2 min() const noexcept
	{
		return mMin;
	}

	/// Retrieves the bbox's maximum point.
	glm::vec2 max() const noexcept
	{
		return mMax;
	}

	/// Retrieves the X range of bbox.
	range x_range() const noexcept
	{
		return _make_range(mMin.x, mMax.x);
	}

	/// Retrieves the Y range of bbox.
	range y_range() const noexcept
	{
		return _make_range(mMin.y, mMax.y);
	}

private:
	glm::vec2 mMin = glm::vec2(std::numeric_limits<glm::float_t>::max());
	glm::vec2 mMax = glm::vec2(std::numeric_limits<glm::float_t>::min());

};


class bbox3 : _bbox_base
{
public:
	/// Builds a null bbox.
	bbox3() = default;

	/// Builds an bbox that encompasses a sphere.
	/// \param[in]  center Center of the sphere.
	/// \param[in]  radius Radius of the sphere.
	bbox3(const glm::vec3& center, glm::float_t radius) noexcept
	{
		include_sphere(center, radius);
	}

	/// Builds an bbox that contains the two points.
	bbox3(const glm::vec3& p1, const glm::vec3& p2) noexcept
	{
		include(p1);
		include(p2);
	}

	/// Returns true if bbox is NULL (not set).
	bool is_null() const noexcept {return mMin.x > mMax.x || mMin.y > mMax.y || mMin.z > mMax.z;}

	/// Extend the bounding box on all sides by \p val.
	void extend(glm::float_t val) noexcept
	{
		assert(!is_null());
		mMin -= glm::vec3(val);
		mMax += glm::vec3(val);

	}

	/// Expand the bbox to include point \p p.
	void include(const glm::vec3& p) noexcept
	{
		mMin = glm::min(p, mMin);
		mMax = glm::max(p, mMax);
	}

	/// Expand the bbox to include a sphere centered at \p center and of radius \p
	/// radius.
	/// \param[in]  center Center of sphere.
	/// \param[in]  radius Radius of sphere.
	void include_sphere(const glm::vec3& p, glm::float_t radius) noexcept
	{
		glm::vec3 r(radius);
		mMin = glm::min(p - r, mMin);
		mMax = glm::max(p + r, mMax);
	}

	/// Expand the bbox to encompass the given \p bbox.
	void include(const bbox3& bbox) noexcept
	{
		include(bbox.mMin);
		include(bbox.mMax);
	}

	/// Translates bbox by vector \p v.
	void translate(const glm::vec3& v) noexcept
	{
		assert(!is_null());
		mMin += v;
		mMax += v;
	}

	/// Returns transformed version of this bbox.
	/// \param[in]  mat  A transformation matrix.
	bbox3 transformed(const glm::mat4& mat) const noexcept;

	/// Scale the bbox by \p scale, centered around \p origin.
	/// \param[in]  scale  3D vector specifying scale along each axis.
	/// \param[in]  origin Origin of scaling operation. Most useful origin would
	///                    be the center of the bbox.
	/// \see scale(const glm::vec3& scale)
	void scale(const glm::vec3& scale, const glm::vec3& origin) noexcept;

	/// Scale the bbox by \p scale, relative to the center of bbox.
	/// \param[in]  scale  3D vector specifying scale along each axis.
	void scale(const glm::vec3& scale_) noexcept
	{
		scale(scale_, center());
	}

	/// Retrieves the center of the bbox.
	glm::vec3 center() const noexcept;

	/// Retrieves the diagonal vector (computed as mMax - mMin).
	/// If the bbox is NULL, then a vector of all zeros is returned.
	glm::vec3 diagonal() const noexcept;

	/// Retrieves the longest edge.
	/// If the bbox is NULL, then 0 is returned.
	glm::float_t longest_edge() const noexcept;

	/// Retrieves the shortest edge.
	/// If the bbox is NULL, then 0 is returned.
	glm::float_t shortest_edge() const noexcept;

	/// Retrieves the bbox's minimum point.
	glm::vec3 min() const noexcept
	{
		return mMin;
	}

	/// Retrieves the bbox's maximum point.
	glm::vec3 max() const noexcept
	{
		return mMax;
	}

	/// Retrieves the X range of bbox.
	range x_range() const noexcept
	{
		return _make_range(mMin.x, mMax.x);
	}

	/// Retrieves the Y range of bbox.
	range y_range() const noexcept
	{
		return _make_range(mMin.y, mMax.y);
	}

	/// Retrieves the Z range of bbox.
	range z_range() const noexcept
	{
		return _make_range(mMin.z, mMax.z);
	}

	/// Type returned from call to intersect.
	enum class Intersection
	{
		Inside,
		Intersect,
		Outside
	};

	/// Returns one of the intersection types. If either of the bboxes are invalid,
	/// then OUTSIDE is returned.
	Intersection intersects(const bbox3& b) const noexcept;

	/// If \p point is outside bbox, returns closest point to \p point on bbox.
	/// Otherwise, returns \p point itself.
	glm::vec3 closest_point(const glm::vec3& point) const noexcept
	{
		return glm::clamp(point, mMin, mMax);
	}

	/// Tests if sphere positioned at \p pos with radius \p r intersects bbox.
	bool intersects_sphere(const glm::vec3& pos, glm::float_t radius) const noexcept;

private:

	glm::vec3 mMin = glm::vec3(std::numeric_limits<glm::float_t>::max());   ///< Minimum point.
	glm::vec3 mMax = glm::vec3(std::numeric_limits<glm::float_t>::min());   ///< Maximum point.
};

}

