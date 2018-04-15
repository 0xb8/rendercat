#pragma once
#include <rendercat/common.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <shaders/common.h>

namespace rc {

namespace Texture
{
	enum class AlphaMode : uint16_t {
		Unknown = 0,
		Opaque  = 1 << 1,
		Mask    = 1 << 2,
		Blend   = 1 << 3
	};

	enum class Kind : uint16_t
	{
		None      = 0,
		Diffuse   = RC_SHADER_TEXTURE_KIND_DIFFUSE,
		Normal    = RC_SHADER_TEXTURE_KIND_NORMAL,
		Specular  = RC_SHADER_TEXTURE_KIND_SPECULAR,
		Roughness = RC_SHADER_TEXTURE_KIND_ROUGNESS,
		Metallic  = RC_SHADER_TEXTURE_KIND_METALLIC,
		Occlusion = RC_SHADER_TEXTURE_KIND_OCCLUSION,
		Emission  = RC_SHADER_TEXTURE_KIND_EMISSION,
	};

	enum class ColorSpace : int8_t
	{
		Linear,
		sRGB
	};

	enum class InternalFormat : uint32_t
	{
		InvalidFormat    = 0,
		KeepParentFormat = 1,
		// values from GL spec
		R_8             = 0x8229,
		R_16            = 0x822A,
		RG_8            = 0x822B,
		RG_16           = 0x822C,
		RGB_8           = 0x8051,
		RGB_10          = 0x8052,
		RGBA_8          = 0x8058,
		SRGB_8          = 0x8C41,
		SRGB_8_ALPHA_8  = 0x8C43,
		R_16F           = 0x822D,
		R_32F           = 0x822E,
		RG_16F          = 0x822F,
		RGB_16F         = 0x881B,
		RGBA_16F        = 0x881A,
		R_11F_G11F_B10F = 0x8C3A,

		// S3TC compressed formats
		Compressed_RGB_DXT1        = 0x83F0,
		Compressed_RGBA_DXT1       = 0x83F1,
		Compressed_RGBA_DXT3       = 0x83F2,
		Compressed_RGBA_DXT5       = 0x83F3,
		Compressed_SRGB_DXT1       = 0x8C4C,
		Compressed_SRGB_ALPHA_DXT1 = 0x8C4D,
		Compressed_SRGB_ALPHA_DXT3 = 0x8C4E,
		Compressed_SRGB_ALPHA_DXT5 = 0x8C4F
	};

	enum class TexelDataType : uint32_t
	{
		UnsignedByte  = 0x1401,
		Byte          = 0x1400,
		UnsignedShort = 0x1403,
		Short         = 0x1402,
		UnsignedInt   = 0x1405,
		Int           = 0x1404,
		HalfFloat     = 0x140B,
		Float         = 0x1406
	};

	enum class MipMapping : std::uint8_t
	{
		Disable,
		MipNearest,
		MipLinear
	};

	enum class Filtering : std::uint8_t
	{
		Nearest,
		Linear
	};

	enum class Wrapping : std::uint8_t
	{
		ClampToBorder,
		ClampToEdge,
		ClampToEdgeMirror,
		Repeat,
		RepeatMirror
	};

	enum class ChannelValue : std::uint8_t
	{
		Zero,
		One,
		Red,
		Green,
		Blue,
		Alpha,
	};

	struct SwizzleMask
	{
		ChannelValue red   = ChannelValue::Red;
		ChannelValue green = ChannelValue::Green;
		ChannelValue blue  = ChannelValue::Blue;
		ChannelValue alpha = ChannelValue::Alpha;
	};

	const char* enum_value_str(AlphaMode) noexcept;
	const char* enum_value_str(ColorSpace) noexcept;
	const char* enum_value_str(ChannelValue) noexcept;
	const char* enum_value_str(Filtering) noexcept;
	const char* enum_value_str(InternalFormat) noexcept;
	const char* enum_value_str(Kind) noexcept;
	const char* enum_value_str(MipMapping) noexcept;
	const char* enum_value_str(Wrapping) noexcept;
}

struct ImageTextureStorage2D
{

	ImageTextureStorage2D() noexcept = default;
	ImageTextureStorage2D(uint16_t width, uint16_t height, Texture::InternalFormat format);
	RC_DEFAULT_MOVE_NOEXCEPT(ImageTextureStorage2D)


	ImageTextureStorage2D share_view(unsigned minlevel,
	                                 unsigned numlevels,
	                                 Texture::InternalFormat view_format = Texture::InternalFormat::KeepParentFormat);

	ImageTextureStorage2D share()
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

	void subImage2D(uint16_t level,
	                uint16_t width,
	                uint16_t height,
	                Texture::TexelDataType type,
	                const void* pixels);

	void compressedSubImage2D(uint16_t level,
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

	uint16_t channels() const noexcept;

	Texture::InternalFormat format() const noexcept
	{
		return m_internal_format;
	}

	Texture::ColorSpace color_space() const noexcept;

	bool valid() const noexcept
	{
		return m_handle.operator bool() && m_internal_format != Texture::InternalFormat::InvalidFormat;
	}

	uint32_t texture_handle() const noexcept
	{
		return *m_handle;
	}


private:
	RC_DISABLE_COPY(ImageTextureStorage2D)
	mutable rc::texture_handle      m_handle;
	Texture::InternalFormat m_internal_format = Texture::InternalFormat::InvalidFormat;
	uint16_t                m_width{};
	uint16_t                m_height{};
	uint8_t                 m_mip_levels{};
	uint8_t                 m_shared{};
};


struct ImageTexture2D
{
	friend class Scene;

	static ImageTexture2D fromFile(const std::string_view path,
	                               Texture::ColorSpace color_space);

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


	bool bindToUnit(uint32_t unit) const noexcept;

	void setFiltering(Texture::MipMapping, Texture::Filtering)  noexcept;
	void setWrapping(Texture::Wrapping) noexcept;

	void setAnisotropy(unsigned samples)  noexcept;
	void setBorderColor(const glm::vec4&) noexcept;
	void setSwizzleMask(const Texture::SwizzleMask&) noexcept;

	ImageTexture2D() = default;
	RC_DEFAULT_MOVE_NOEXCEPT(ImageTexture2D)

private:
	RC_DISABLE_COPY(ImageTexture2D)

	void setAllParams();

	ImageTextureStorage2D m_storage;
	glm::vec4             m_border_color{};
	Texture::SwizzleMask  m_swizzle_mask{};
	Texture::MipMapping   m_mipmapping = Texture::MipMapping::MipLinear;
	Texture::Filtering    m_filtering  = Texture::Filtering::Linear;
	Texture::Wrapping     m_wrapping = Texture::Wrapping::Repeat;
	uint8_t               m_anisotropic_samples = 16; // min value required by spec
};

} // namespace rc
