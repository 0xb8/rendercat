#pragma once

#include <cstdint>
#include <zcm/vec2.hpp>
#include <zcm/vec3.hpp>
#include <zcm/mat4.hpp>

#define DEBUG_DRAW_VEC3_TYPE_DEFINED
using ddVec3    = zcm::vec3;
using ddVec3_In  = const ddVec3&;
using ddVec3_Out = ddVec3&;

#include "debug-draw/debug_draw.hpp"
