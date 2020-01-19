#pragma once

#include <zcm/vec3.hpp>

namespace rc::util {

	zcm::vec3 temperature_to_srgb_color(float kelvin);

	zcm::vec3 temperature_to_linear_color(float kelvin);

}
