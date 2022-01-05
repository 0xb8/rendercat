#pragma once
#include <stdint.h>
#include <filesystem>

namespace rc::util {

void gl_screenshot(unsigned w, unsigned h, const std::filesystem::path& file);

void gl_save_hdr_texture(uint32_t tex, const std::filesystem::path& file);

}
