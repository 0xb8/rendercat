#pragma once

#include <zcm/vec2.hpp>
#include <zcm/vec3.hpp>
#include <zcm/common.hpp>
#include <rendercat/core/ray.hpp>
#include <limits>
#include <cassert>
#include <type_traits>

namespace zcm {
	struct mat3;
	struct mat4;
	struct quat;
}

namespace rc {

/// Type returned from call to intersect.
enum class Intersection
{
	Outside = 0,
	Intersect = 1,
	Inside = 2,
};

#ifdef __cpp_fold_expressions

namespace _bbox_detail {

	// wrapper holding a value
	template <typename T>
	struct vminmax_wrapper {
		T value;
	};

	// min operator
	template <typename T, typename U, typename V=std::common_type_t<T,U>>
	inline constexpr vminmax_wrapper<V> operator<<=(const vminmax_wrapper<T>& lhs, const vminmax_wrapper<U>& rhs) noexcept
	{
		return {zcm::min(lhs.value, rhs.value)};
	}

	// max operator
	template <typename T, typename U, typename V=std::common_type_t<T,U>>
	inline constexpr vminmax_wrapper<V> operator>>=(const vminmax_wrapper<T>& lhs, const vminmax_wrapper<U>& rhs) noexcept
	{
		return {zcm::max(lhs.value, rhs.value)};
	}

	template <typename... Ts>
	inline constexpr auto min(Ts&&... args) noexcept {
		return (vminmax_wrapper<Ts>{args} <<= ...).value;
	}

	template <typename... Ts>
	inline constexpr auto max(Ts&&... args) noexcept {
		return (vminmax_wrapper<Ts>{args} >>= ...).value;
	}
}

#endif


class bbox2
{
public:

	/// Range of axis' coordinates.
	struct range
	{
		float min;
		float max;
	};


	/// Builds a null bbox.
	bbox2() = default;

	/// Builds an bbox that contains the two points.
	bbox2(const zcm::vec2& p1, zcm::vec2& p2) noexcept
	{
		include(p1);
		include(p2);
	}

	/// Returns true if bbox is NULL (not set), or bbox is invalid (has nan).
	bool is_null() const noexcept {return !(mMin.x <= mMax.x && mMin.y <= mMax.y);}

	/// Expand the bbox to include point \p p.
	void include(const zcm::vec2& p) noexcept
	{
		mMin = zcm::min(mMin, p);
		mMax = zcm::max(mMax, p);
	}

#ifdef __cpp_fold_expressions

	/// Expand the bbox to include all of the \p points.
	template<typename... Ts>
	void include(const zcm::vec2& p1, const Ts&... points) noexcept
	{
		mMin = _bbox_detail::min(mMin, p1, points...);
		mMax = _bbox_detail::max(mMax, p1, points...);
	}

#endif

	/// Expand the bbox to encompass the given \p bbox.
	void include(const bbox2& bbox) noexcept
	{
		mMin = zcm::min(mMin, bbox.mMin);
		mMax = zcm::max(mMax, bbox.mMax);
	}

	/// Expand the bbox to include a circle centered at \p center and of radius \p
	/// radius.
	/// \param[in]  center Center of circle.
	/// \param[in]  radius Radius of circle.
	void include_circle(const zcm::vec2& p, float radius) noexcept
	{
		zcm::vec2 r(radius);
		mMin = zcm::min(mMin, p - r);
		mMax = zcm::max(mMax, p + r);
	}

	/// Extend the bounding box on all sides by \p val.
	void extend(float val) noexcept
	{
		assert(!is_null());
		assert(!zcm::isnan(val));
		mMin -= zcm::vec2(val);
		mMax += zcm::vec2(val);
	}

	/// Translates bbox by vector \p v.
	void translate(const zcm::vec2& v) noexcept
	{
		assert(!is_null()); // TODO: nan-safety
		mMin += v;
		mMax += v;
	}

	/// Scale the bbox by \p scale, centered around \p origin.
	/// \param[in]  scale  2D vector specifying scale along each axis.
	/// \param[in]  origin Origin of scaling operation. Most useful origin would
	///                    be the center of the bbox.
	/// \see scale(const zcm::vec2& scale)
	void scale(const zcm::vec2& scale, const zcm::vec2& origin) noexcept;

	/// Scale the bbox by \p scale, relative to the center of bbox.
	/// \param[in]  scale  2D vector specifying scale along each axis.
	void scale(const zcm::vec2& scale_) noexcept
	{
		scale(scale_, center());
	}

	/// Returns transformed version of this bbox.
	/// \param[in]  mat  A transformation matrix.
	static bbox2 transformed(const bbox2& bbox, const zcm::mat3& mat) noexcept;

	/// Retrieves the center of the bbox.
	zcm::vec2 center() const noexcept;

	/// Retrieves the diagonal vector (computed as mMax - mMin).
	/// If the bbox is NULL, then a vector of all zeros is returned.
	zcm::vec2 diagonal() const noexcept;

	/// Retrieves the longest edge.
	/// If the bbox is NULL, then 0 is returned.
	float longest_edge() const noexcept;

	/// Retrieves the shortest edge.
	/// If the bbox is NULL, then 0 is returned.
	float shortest_edge() const noexcept;

	/// Retrieves the bbox's minimum point.
	zcm::vec2 min() const noexcept
	{
		return mMin;
	}

	/// Retrieves the bbox's maximum point.
	zcm::vec2 max() const noexcept
	{
		return mMax;
	}

	/// Retrieves the X range of bbox.
	range x_range() const noexcept
	{
		return range{mMin.x, mMax.x};
	}

	/// Retrieves the Y range of bbox.
	range y_range() const noexcept
	{
		return range{mMin.y, mMax.y};
	}

	/// If \p point is outside bbox, returns closest point to \p point on bbox.
	/// Otherwise, returns \p point itself.
	zcm::vec2 closest_point(zcm::vec2 point) const noexcept
	{
		return zcm::clamp(point, mMin, mMax);
	}


	/// Returns one of the intersection types. If either of the bboxes are invalid,
	/// then OUTSIDE is returned.
	static Intersection intersects(const bbox2& a, const bbox2& b) noexcept;

	/// Tests if sphere positioned at \p pos with radius \p r intersects bbox.
	static Intersection intersects_sphere(const bbox2& bbox, const zcm::vec2& center, float radius) noexcept;

	/// Tests if \p ray intersects \p bbox.
	static Intersection intersects_ray(const bbox2& bbox, const ray2_inv& ray) noexcept;

private:
	zcm::vec2 mMin = zcm::vec2(std::numeric_limits<float>::max());    ///< Minimum point.
	zcm::vec2 mMax = zcm::vec2(std::numeric_limits<float>::lowest()); ///< Maximum point.

};


class bbox3
{
public:

	/// Range of axis' coordinates.
	struct range
	{
		float min;
		float max;
	};

	/// Builds a null bbox.
	bbox3() = default;

	/// Builds an bbox that contains the two points.
	bbox3(const zcm::vec3& p1, const zcm::vec3& p2) noexcept
	{
		include(p1);
		include(p2);
	}

	/// Returns true if bbox is NULL (not set).
	bool is_null() const noexcept {return !(mMin.x <= mMax.x && mMin.y <= mMax.y && mMin.z <= mMax.z);}

	/// Extend the bounding box on all sides by \p val.
	void extend(float val) noexcept
	{
		assert(!is_null());
		assert(!zcm::isnan(val));
		mMin -= zcm::vec3(val);
		mMax += zcm::vec3(val);

	}

	/// Expand the bbox to include point \p p.
	void include(const zcm::vec3& p) noexcept
	{
		mMin = zcm::min(mMin, p);
		mMax = zcm::max(mMax, p);
	}

#ifdef __cpp_fold_expressions

	/// Expand the bbox to include all of the \p points.
	template<typename... Ts>
	void include(const zcm::vec3& p1, const Ts&... points) noexcept
	{
		mMin = _bbox_detail::min(mMin, p1, points...);
		mMax = _bbox_detail::max(mMax, p1, points...);
	}

#endif

	/// Expand the bbox to encompass the given \p bbox.
	void include(const bbox3& bbox) noexcept
	{
		mMin = zcm::min(mMin, bbox.mMin);
		mMax = zcm::max(mMax, bbox.mMax);
	}

	/// Expand the bbox to include a sphere centered at \p center and of radius \p
	/// radius.
	/// \param[in]  center Center of sphere.
	/// \param[in]  radius Radius of sphere.
	void include_sphere(const zcm::vec3& p, float radius) noexcept
	{
		zcm::vec3 r(radius);
		mMin = zcm::min(mMin, p - r);
		mMax = zcm::max(mMax, p + r);
	}

	/// Translates bbox by vector \p v.
	void translate(const zcm::vec3& v) noexcept
	{
		assert(!is_null());
		mMin += v;
		mMax += v;
	}

	/// Returns transformed version of this bbox.
	/// \param[in]  mat  A transformation matrix.
	static bbox3 transformed(const bbox3& bbox, const zcm::mat4& mat) noexcept;

	/// Returns rotated version of this bbox.
	/// \param[in]  quat  A unit rotation quaternion.
	static bbox3 rotated(const bbox3& bbox, const zcm::quat& quat) noexcept;

	/// Scale the bbox by \p scale, centered around \p origin.
	/// \param[in]  scale  3D vector specifying scale along each axis.
	/// \param[in]  origin Origin of scaling operation. Most useful origin would
	///                    be the center of the bbox.
	/// \see scale(const zcm::vec3& scale)
	void scale(const zcm::vec3& scale, const zcm::vec3& origin) noexcept;

	/// Scale the bbox by \p scale, relative to the center of bbox.
	/// \param[in]  scale  3D vector specifying scale along each axis.
	void scale(const zcm::vec3& scale_) noexcept
	{
		scale(scale_, center());
	}

	/// Retrieves the center of the bbox.
	zcm::vec3 center() const noexcept;

	/// Retrieves the diagonal vector (computed as mMax - mMin).
	/// If the bbox is NULL, then a vector of all zeros is returned.
	zcm::vec3 diagonal() const noexcept;

	/// Retrieves the longest edge.
	/// If the bbox is NULL, then 0 is returned.
	float longest_edge() const noexcept;

	/// Retrieves the shortest edge.
	/// If the bbox is NULL, then 0 is returned.
	float shortest_edge() const noexcept;

	/// Retrieves the bbox's minimum point.
	zcm::vec3 min() const noexcept
	{
		return mMin;
	}

	/// Retrieves the bbox's maximum point.
	zcm::vec3 max() const noexcept
	{
		return mMax;
	}

	/// Retrieves the X range of bbox.
	range x_range() const noexcept
	{
		return range{mMin.x, mMax.x};
	}

	/// Retrieves the Y range of bbox.
	range y_range() const noexcept
	{
		return range{mMin.y, mMax.y};
	}

	/// Retrieves the Z range of bbox.
	range z_range() const noexcept
	{
		return range{mMin.z, mMax.z};
	}

	/// If \p point is outside bbox, returns closest point to \p point on bbox.
	/// Otherwise, returns \p point itself.
	zcm::vec3 closest_point(const zcm::vec3& point) const noexcept
	{
		return zcm::clamp(point, mMin, mMax);
	}

	/// Returns one of the intersection types. If either of the bboxes are invalid,
	/// then OUTSIDE is returned.
	static Intersection intersects(const bbox3& a, const bbox3& b) noexcept;

	/// Tests if sphere positioned at \p pos with radius \p r intersects bbox.
	static Intersection intersects_sphere(const bbox3& bbox, const zcm::vec3& center, float radius) noexcept;

	/// Tests if \p ray intersects \p bbox.
	static Intersection intersects_ray(const bbox3& bbox, const ray3_inv& ray) noexcept;

private:

	zcm::vec3 mMin = zcm::vec3(std::numeric_limits<float>::max());    ///< Minimum point.
	zcm::vec3 mMax = zcm::vec3(std::numeric_limits<float>::lowest()); ///< Maximum point.
};

}

