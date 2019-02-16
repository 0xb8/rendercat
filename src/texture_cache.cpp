#include <rendercat/texture_cache.hpp>
#include <unordered_map>


class TextureCache
{
	TextureCache() = default;
	RC_DISABLE_COPY(TextureCache)
	RC_DISABLE_MOVE(TextureCache)
	public:

	static TextureCache& instance()
	{
		static TextureCache cache;
		return cache;
	}

	void clear()
	{
		m_cache.clear();
	}

	void add(std::string&& path, rc::ImageTexture2D&& tex)
	{
		m_cache.emplace(std::make_pair(std::move(path), std::move(tex)));
	}

	rc::ImageTexture2D* get(const std::string& path)
	{
		auto pos = m_cache.find(path);
		if(pos != m_cache.end())
			return &pos->second;
		return nullptr;
	}

private:
	std::unordered_map<std::string, rc::ImageTexture2D> m_cache;
};


void rc::Texture::Cache::clear()
{
	TextureCache::instance().clear();
}

void rc::Texture::Cache::add(std::string && path, rc::ImageTexture2D && tex)
{
	TextureCache::instance().add(std::move(path), std::move(tex));
}

rc::ImageTexture2D* rc::Texture::Cache::get(const std::string & path)
{
	return TextureCache::instance().get(path);
}
