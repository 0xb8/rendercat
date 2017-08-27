#pragma once
#include <rendercat/common.hpp>

class TextureCache;

struct Material
{
	friend class Scene;
	enum Flags
	{
		Opaque          = 1 << 1,
		Masked          = 1 << 2,
		Blended         = 1 << 3,

		NormalMapped    = 1 << 8,
		SpecularMapped  = 1 << 9,
		RoughnessMapped = 1 << 10,
		MetallicMapped  = 1 << 11,

		FaceCullingDisabled = 1 << 24
	};

	static void set_texture_cache(TextureCache* c);

	static void set_default_diffuse() noexcept;

	Material(const std::string& name_) noexcept : name(name_), m_diffuse_map(default_diffuse){}
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
		flags            = o.flags;

		name = std::move(o.name);
		std::swap(m_diffuse_map, o.m_diffuse_map);
		std::swap(m_normal_map, o.m_normal_map);
		std::swap(m_specular_map, o.m_specular_map);

		return *this;
	}

	void bind(GLuint s) const noexcept;

	void specularColorShininess(glm::vec3 color, float shininess = 8.0f) noexcept
	{
		m_specular_color = color;
		m_shininess = shininess;
	}

	void addDiffuseMap(const std::string_view name, const std::string_view basedir, bool alpha_masked);
	void addNormalMap(const std::string_view name, const std::string_view basedir);
	void addSpecularMap(const std::string_view name, const std::string_view basedir);


	std::string name;
	Flags       flags = Flags::Opaque;

private:
	static TextureCache* cache;
	static uint32_t default_diffuse;


	glm::vec3 m_specular_color = {0.0f, 0.0f, 0.0f};
	float     m_shininess      = 0.0f;

	uint32_t  m_diffuse_map  = 0;
	uint32_t  m_normal_map   = 0;
	uint32_t  m_specular_map = 0;
};


inline constexpr Material::Flags operator|(Material::Flags a, Material::Flags b) noexcept
{
	return static_cast<Material::Flags>(+a | +b);
}
inline constexpr void operator|=(Material::Flags& a, Material::Flags b) noexcept
{
	a = static_cast<Material::Flags>(+a | +b);
}

inline constexpr Material::Flags operator&(Material::Flags a, Material::Flags b) noexcept
{
	return static_cast<Material::Flags>(+a & +b);
}

inline constexpr void clear(Material::Flags& a, Material::Flags b) noexcept
{
	a = static_cast<Material::Flags>(+a & ~(+b));
}
