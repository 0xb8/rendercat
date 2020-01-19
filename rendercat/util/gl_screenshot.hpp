#pragma once
#include <stdint.h>
#include <string_view>

namespace rc::util {

void gl_screenshot(unsigned w, unsigned h, std::string_view file);

void gl_save_hdr_texture(uint32_t tex, std::string_view file);

}
