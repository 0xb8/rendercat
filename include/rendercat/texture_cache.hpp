#pragma once

#include <rendercat/texture2d.hpp>

namespace rc {
namespace Texture {
namespace Cache {

void clear();

void add(std::string&& path, rc::ImageTexture2D&& tex);

ImageTexture2D* get(const std::string& path);


}
}
}
