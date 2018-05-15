#pragma once
#include <rendercat/common.hpp>
#include <rendercat/texture.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <vector>

namespace rc {

// A shareable 2D texture memory representation.
struct TextureStorage2D
{
	TextureStorage2D() noexcept = default;
	TextureStorage2D(uint16_t width, uint16_t height, Texture::InternalFormat format);

	RC_DEFAULT_MOVE_NOEXCEPT(TextureStorage2D)
	RC_DISABLE_COPY(TextureStorage2D)

	TextureStorage2D share_view(unsigned minlevel,
	                            unsigned numlevels,
	                            Texture::InternalFormat view_format = Texture::InternalFormat::KeepParentFormat);

	TextureStorage2D share()
	{
		return share_view(0, m_mip_levels);
	}

	bool shared() const noexcept
	{
		return m_shared;
	}

	int share_count() const noexcept
	{
		return m_shared;
	}

	void sub_image(uint16_t level,
	               uint16_t width,
	               uint16_t height,
	               Texture::TexelDataType type,
	               const void* pixels);

	void compressed_sub_image(uint16_t level,
	                          uint16_t width,
	                          uint16_t height,
	                          size_t data_len,
	                          const void* data);

	uint16_t levels() const noexcept
	{
		return m_mip_levels;
	}

	uint16_t width()  const noexcept
	{
		return m_width;
	}

	uint16_t height() const noexcept
	{
		return m_height;
	}


	Texture::InternalFormat format() const noexcept
	{
		return m_internal_format;
	}

	bool valid() const noexcept
	{
		return m_handle.operator bool() && m_internal_format != Texture::InternalFormat::InvalidFormat;
	}

	uint32_t texture_handle() const noexcept
	{
		return *m_handle;
	}


	uint16_t channels() const noexcept;
	Texture::ColorSpace color_space() const noexcept;

private:
	mutable rc::texture_handle m_handle;
	Texture::InternalFormat    m_internal_format = Texture::InternalFormat::InvalidFormat;
	uint16_t                   m_width{};
	uint16_t                   m_height{};
	uint8_t                    m_mip_levels{};
	uint8_t                    m_shared{};
};

//------------------------------------------------------------------------------

struct ImageTexture2D
{
	ImageTexture2D() = default;

	static ImageTexture2D fromFile(const std::string_view file, Texture::ColorSpace colorspace);

	ImageTexture2D share();

	bool shared() const noexcept
	{
		return m_storage.shared();
	}

	bool valid() const noexcept
	{
		return m_storage.valid();
	}

	uint32_t texture_handle() const noexcept
	{
		return m_storage.texture_handle();
	}

	uint16_t width() const noexcept
	{
		return m_storage.width();
	}

	uint16_t height() const noexcept
	{
		return m_storage.height();
	}

	uint16_t levels() const noexcept
	{
		return m_storage.levels();
	}

	uint16_t channels() const noexcept
	{
		return m_storage.channels();
	}

	const TextureStorage2D& storage() const noexcept
	{
		return m_storage;
	}

	Texture::SwizzleMask swizzle_mask() const noexcept
	{
		return m_swizzle_mask;
	}

	Texture::MipMapping mipmapping() const noexcept
	{
		return m_mipmapping;
	}

	Texture::Wrapping wrapping() const noexcept
	{
		return m_wrapping;
	}

	uint16_t anisotropy() const noexcept
	{
		return m_anisotropic_samples;
	}

	Texture::Filtering filtering() const noexcept
	{
		return m_filtering;
	}

	glm::vec4 border_color() const noexcept
	{
		return m_border_color;
	}

	bool bind_to_unit(uint32_t unit) const noexcept;

	void set_filtering(Texture::MipMapping, Texture::Filtering)  noexcept;
	void set_wrapping(Texture::Wrapping) noexcept;

	void set_anisotropy(unsigned samples)  noexcept;
	void set_border_color(const glm::vec4&) noexcept;
	void set_swizzle_mask(const Texture::SwizzleMask&) noexcept;

private:
	void set_default_params();

	TextureStorage2D      m_storage;
	glm::vec4             m_border_color{};
	Texture::SwizzleMask  m_swizzle_mask{};
	Texture::MipMapping   m_mipmapping = Texture::MipMapping::MipLinear;
	Texture::Filtering    m_filtering  = Texture::Filtering::Linear;
	Texture::Wrapping     m_wrapping = Texture::Wrapping::Repeat;
	uint8_t               m_anisotropic_samples = 16; // min value required by spec
};

} // namespace rc
