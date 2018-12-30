#pragma once

#include <zcm/vec2.hpp>
#include <zcm/vec3.hpp>

namespace rc {

struct ray2
{
	zcm::vec2 origin;
	zcm::vec2 dir;
};

struct ray3
{
	zcm::vec3 origin;
	zcm::vec3 dir;
};

struct ray2_inv
{
	zcm::vec2 origin;
	zcm::vec2 dir_inv;
};
struct ray3_inv
{
	zcm::vec3 origin;
	zcm::vec3 dir_inv;
};

inline ray2_inv inv_normal(const ray2& ray) noexcept
{
	return {ray.origin, zcm::vec2(1.0f) / ray.dir};
}

inline ray3_inv inv_normal(const ray3& ray) noexcept
{
	return {ray.origin, zcm::vec3(1.0f) / ray.dir};
}

}
