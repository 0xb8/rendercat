#pragma once
#include <unordered_map>
#include <rendercat/texture2d.hpp>
namespace rc {

struct TextureCache
{
	TextureCache() = default;
	RC_DISABLE_COPY(TextureCache)
	RC_DEFAULT_MOVE(TextureCache)

	void add(std::string&& path, ImageTexture2D&& tex)
	{
		cache.emplace(std::make_pair(std::move(path), std::move(tex)));
	}

	ImageTexture2D* get(const std::string& path)
	{
		auto pos = cache.find(path);
		if(pos != cache.end())
			return &pos->second;
		return nullptr;
	}

	void clear()
	{
		cache.clear();
	}

private:
	std::unordered_map<std::string, ImageTexture2D> cache;
};

}
