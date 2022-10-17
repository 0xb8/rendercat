#pragma once

#include <rendercat/texture2d.hpp>

namespace rc::Texture::Cache {

void clear();

void add(const std::filesystem::path&, rc::ImageTexture2D&& tex);

ImageTexture2D* get(const std::filesystem::path& path);

}
