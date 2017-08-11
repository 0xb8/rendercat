#pragma once
#include <unordered_map>
#include <common.hpp>

struct TextureCache
{
	struct Result
	{
		uint32_t to = 0;
		int num_channels = 0;
	};

	TextureCache() = default;

	void add(std::string&& path, uint32_t to, int numchan);
	Result get(const std::string& path);
private:
	std::unordered_map<std::string, Result> cache;
};


struct Material
{
	friend class Scene;
	enum Type
	{
		Opaque         = 1 << 1,
		Masked         = 1 << 2,
		Transparent    = 1 << 3,
		NormalMapped   = 1 << 8,
		SpecularMapped = 1 << 9
	};

	static void set_texture_cache(TextureCache* c);

	static void set_default_diffuse() noexcept;

	Material(const std::string& name) noexcept : m_name(name), m_diffuse_map(default_diffuse)
	{
	}

	~Material() noexcept;

	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;

	Material(Material&& o) noexcept
	{
		this->operator=(std::move(o));
	}
	Material& operator =(Material&& o) noexcept
	{
		m_specular_color = o.m_specular_color;
		m_shininess      = o.m_shininess;
		m_type           = o.m_type;

		std::swap(m_name, o.m_name);
		std::swap(m_diffuse_map, o.m_diffuse_map);
		std::swap(m_normal_map, o.m_normal_map);
		std::swap(m_specular_map, o.m_specular_map);

		return *this;
	}

	Type type() const noexcept
	{
		return (Type)m_type;
	}

	void bind(GLuint s) const noexcept;

	void specularColorShininess(glm::vec3 color, float shininess = 8.0f) noexcept
	{
		m_specular_color = color;
		m_shininess = shininess;
	}

	void addDiffuseMap(std::string_view name, std::string_view basedir);
	void addNormalMap(std::string_view name, std::string_view basedir);
	void addSpecularMap(std::string_view name, std::string_view basedir);

private:
	static TextureCache* cache;
	static uint32_t default_diffuse;

	std::string m_name;
	glm::vec3 m_specular_color = {0.0f, 0.0f, 0.0f};
	float     m_shininess      = 0.0f;

	int  m_type         = Type::Opaque;
	uint32_t  m_diffuse_map  = 0;
	uint32_t  m_normal_map   = 0;
	uint32_t  m_specular_map = 0;
};
