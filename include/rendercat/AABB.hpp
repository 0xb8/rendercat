#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

/// Standalone axis aligned bounding box implemented built on top of GLM.
class AABB
{
public:
	/// Builds a null AABB.
	AABB() = default;

	/// Builds an AABB that encompasses a sphere.
	/// \param[in]  center Center of the sphere.
	/// \param[in]  radius Radius of the sphere.
	AABB(const glm::vec3& center, glm::float_t radius) : AABB()
	{
		include_sphere(center, radius);
	}

	/// Builds an AABB that contains the two points.
	AABB(const glm::vec3& p1, const glm::vec3& p2) : AABB()
	{
		include(p1);
		include(p2);
	}

	AABB(const AABB& aabb) : AABB()
	{
		include(aabb);
	}

	/// Returns true if AABB is NULL (not set).
	bool is_null() const {return mMin.x > mMax.x || mMin.y > mMax.y || mMin.z > mMax.z;}

	/// Extend the bounding box on all sides by \p val.
	void extend(glm::float_t val)
	{
		assert(!is_null());
		mMin -= glm::vec3(val);
		mMax += glm::vec3(val);

	}

	/// Expand the AABB to include point \p p.
	void include(const glm::vec3& p)
	{
		if (!is_null()) {
			mMin = glm::min(p, mMin);
			mMax = glm::max(p, mMax);
		} else {
			mMin = p;
			mMax = p;
		}
	}

	/// Expand the AABB to include a sphere centered at \p center and of radius \p
	/// radius.
	/// \param[in]  center Center of sphere.
	/// \param[in]  radius Radius of sphere.
	void include_sphere(const glm::vec3& p, glm::float_t radius)
	{
		glm::vec3 r(radius);
		if (!is_null()) {
			mMin = glm::min(p - r, mMin);
			mMax = glm::max(p + r, mMax);
		} else {
			mMin = p - r;
			mMax = p + r;
		}
	}

	/// Expand the AABB to encompass the given \p aabb.
	void include(const AABB& aabb)
	{
		if (!aabb.is_null()) {
			include(aabb.mMin);
			include(aabb.mMax);
		}
	}

	/// Translates AABB by vector \p v.
	void translate(const glm::vec3& v)
	{
		assert(!is_null());
		mMin += v;
		mMax += v;

	}

	/// Scale the AABB by \p scale, centered around \p origin.
	/// \param[in]  scale  3D vector specifying scale along each axis.
	/// \param[in]  origin Origin of scaling operation. Most useful origin would
	///                    be the center of the AABB.
	void scale(const glm::vec3& scale, const glm::vec3& origin)
	{
		assert(!is_null());
		mMin -= origin;
		mMax -= origin;

		mMin *= scale;
		mMax *= scale;

		mMin += origin;
		mMax += origin;

	}

	/// Retrieves the center of the AABB.
	glm::vec3 center() const
	{
		if (!is_null()) {
			glm::vec3 d = diagonal();
			return mMin + (d * glm::float_t(0.5));
		} else {
			return glm::vec3(0.0);
		}
	}

	/// Retrieves the diagonal vector (computed as mMax - mMin).
	/// If the AABB is NULL, then a vector of all zeros is returned.
	glm::vec3 diagonal() const
	{
		if (!is_null())
			return mMax - mMin;
		else
			return glm::vec3(0);
	}

	/// Retrieves the longest edge.
	/// If the AABB is NULL, then 0 is returned.
	glm::float_t longest_edge() const
	{
		return glm::compMax(diagonal());
	}

	/// Retrieves the shortest edge.
	/// If the AABB is NULL, then 0 is returned.
	glm::float_t shortest_edge() const
	{
		return glm::compMin(diagonal());
	}

	/// Retrieves the AABB's minimum point.
	glm::vec3 min() const
	{
		return mMin;
	}

	/// Retrieves the AABB's maximum point.
	glm::vec3 max() const
	{
		return mMax;
	}

	/// Type returned from call to intersect.
	enum class Intersection
	{
		Inside,
		Intersect,
		Outside
	};

	/// Returns one of the intersection types. If either of the aabbs are invalid,
	/// then OUTSIDE is returned.
	Intersection intersects(const AABB& b) const
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

	/// If \p point is outside AABB, returns closest point to \p point on AABB.
	/// Otherwise, returns \p point itself.
	glm::vec3 closest_point(const glm::vec3& point) const
	{
		return glm::clamp(point, mMin, mMax);
	}

	/// Tests if sphere positioned at \p pos with radius \p r intersects AABB.
	bool intersects_sphere(const glm::vec3& pos, float r) const
	{
		double distance = glm::length(pos - closest_point(pos));
		return distance*distance < r * r;
	}

private:

	glm::vec3 mMin = glm::vec3(1.0);   ///< Minimum point.
	glm::vec3 mMax = glm::vec3(-1.0);  ///< Maximum point.
};


