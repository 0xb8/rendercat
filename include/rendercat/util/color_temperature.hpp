#pragma once

#include <glm/vec3.hpp>

namespace rc { namespace util {

	glm::vec3 temperature_to_srgb_color(float kelvin);

	glm::vec3 temperature_to_linear_color(float kelvin);

}}
