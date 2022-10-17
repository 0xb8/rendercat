#pragma once
#include <rendercat/common.hpp>
#include <rendercat/texture.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <string_view>
#include <filesystem>
#include <zcm/vec4.hpp>

namespace rc {

// A shareable 2D texture memory representation.
struct TextureStorage2D
{
	TextureStorage2D() noexcept = default;
	TextureStorage2D(uint16_t width, uint16_t height, Texture::InternalFormat format, int16_t levels = -1);

	RC_DEFAULT_MOVE_NOEXCEPT(TextureStorage2D)
	RC_DISABLE_COPY(TextureStorage2D)

	[[nodiscard]] TextureStorage2D share_view(unsigned minlevel,
	                                          unsigned numlevels,
	                                          Texture::InternalFormat view_format = Texture::InternalFormat::KeepParentFormat);

	[[nodiscard]] TextureStorage2D share();

	[[nodiscard]] bool shared() const noexcept;
	[[nodiscard]] int  ref_count() const noexcept;

	void reset() noexcept;
	void set_label(std::string_view label);
	std::string label() const;

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

	[[nodiscard]] bool valid() const noexcept;

	uint16_t levels() const noexcept;
	uint16_t channels() const noexcept;
	uint16_t width()  const noexcept;
	uint16_t height() const noexcept;

	Texture::ColorSpace color_space() const noexcept;
	Texture::InternalFormat format() const noexcept;
	uint32_t texture_handle() const noexcept;

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

	[[nodiscard]] static ImageTexture2D fromFile(const std::filesystem::path& file, Texture::ColorSpace colorspace);

	[[nodiscard]] ImageTexture2D share();

	[[nodiscard]] bool shared() const noexcept;
	[[nodiscard]] bool valid() const noexcept;
	void reset() noexcept;

	uint32_t texture_handle() const noexcept;

	uint16_t width() const noexcept;
	uint16_t height() const noexcept;
	uint16_t levels() const noexcept;
	uint16_t channels() const noexcept;
	uint16_t anisotropy() const noexcept;
	zcm::vec4 border_color() const noexcept;
	float mip_bias() const noexcept;

	const TextureStorage2D& storage() const noexcept;

	Texture::SwizzleMask swizzle_mask() const noexcept;
	Texture::MagFilter mag_filter() const noexcept;
	Texture::MinFilter min_filter() const noexcept;
	Texture::Wrapping wrapping_s() const noexcept;
	Texture::Wrapping wrapping_t() const noexcept;

	void set_filtering(Texture::MinFilter, Texture::MagFilter)  noexcept;
	void set_wrapping(Texture::Wrapping s, Texture::Wrapping t) noexcept;

	void set_anisotropy(unsigned samples)  noexcept;
	void set_border_color(const zcm::vec4&) noexcept;
	void set_swizzle_mask(const Texture::SwizzleMask&) noexcept;
	void set_mip_bias(float bias) noexcept;

private:
	void set_default_params();

	TextureStorage2D      m_storage;
	zcm::vec4             m_border_color{};
	float                 m_bias = 0.0f;
	Texture::SwizzleMask  m_swizzle_mask{};
	Texture::MagFilter    m_mag_filter = Texture::MagFilter::Linear;
	Texture::MinFilter    m_min_filter = Texture::MinFilter::LinearMipMapLinear;
	Texture::Wrapping     m_wrapping_s = Texture::Wrapping::Repeat;
	Texture::Wrapping     m_wrapping_t = Texture::Wrapping::Repeat;
	uint8_t               m_anisotropic_samples = 16; // min value required by spec
};

namespace Texture {
	bool bind_to_unit(const ImageTexture2D& texture, uint32_t unit) noexcept;
}

} // namespace rc
