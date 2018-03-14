#pragma once

#define GLM_FORCE_CXX14
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace rc {

struct ray2
{
	glm::vec2 origin;
	glm::vec2 dir;
};

struct ray3
{
	glm::vec3 origin;
	glm::vec3 dir;
};

struct ray2_inv
{
	glm::vec2 origin;
	glm::vec2 dir_inv;
};
struct ray3_inv
{
	glm::vec3 origin;
	glm::vec3 dir_inv;
};

inline ray2_inv inv_normal(const ray2& ray) noexcept
{
	return {ray.origin, glm::vec2(1.0f) / ray.dir};
}

inline ray3_inv inv_normal(const ray3& ray) noexcept
{
	return {ray.origin, glm::vec3(1.0f) / ray.dir};
}

}
