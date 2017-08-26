#pragma once
#include <unordered_map>

struct TextureCache
{
	struct Result
	{
		uint32_t to = 0;
		int num_channels = 0;
	};

	TextureCache() = default;
	TextureCache(const TextureCache&) = delete;
	TextureCache(TextureCache&&)      = default;

	TextureCache& operator=(const TextureCache&) = delete;
	TextureCache& operator=(TextureCache&&)      = default;

	void add(std::string&& path, uint32_t to, int numchan);
	Result get(const std::string& path);

	
private:
	std::unordered_map<std::string, Result> cache;
};