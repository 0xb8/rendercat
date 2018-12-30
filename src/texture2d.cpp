#include <rendercat/texture2d.hpp>
#include <stb_image.h>
#include <fmt/core.h>

#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding/gl45ext/enum.h>

#include <zcm/common.hpp>
#include <zcm/exponential.hpp>
#include <zcm/type_ptr.hpp>

using namespace gl45core;
using namespace rc;

//------------------------------------------------------------------------------


static GLenum components(Texture::InternalFormat format)
{
	using namespace Texture;
	switch (format) {
	case InternalFormat::R_8:
	case InternalFormat::R_16:
	case InternalFormat::R_16F:
	case InternalFormat::R_32F:
		return GL_RED;

	case InternalFormat::RG_8:
	case InternalFormat::RG_16:
	case InternalFormat::RG_16F:
		return GL_RG;

	case InternalFormat::RGB_8:
	case InternalFormat::SRGB_8:
	case InternalFormat::RGB_10:
	case InternalFormat::RGB_16F:
	case InternalFormat::R_11F_G11F_B10F:
	case InternalFormat::Compressed_RGB_DXT1:
	case InternalFormat::Compressed_SRGB_DXT1:
		return GL_RGB;

	case InternalFormat::RGBA_8:
	case InternalFormat::SRGB_8_ALPHA_8:
	case InternalFormat::RGBA_16F:
	case InternalFormat::Compressed_RGBA_DXT1:
	case InternalFormat::Compressed_RGBA_DXT3:
	case InternalFormat::Compressed_RGBA_DXT5:
	case InternalFormat::Compressed_SRGB_ALPHA_DXT1:
	case InternalFormat::Compressed_SRGB_ALPHA_DXT3:
	case InternalFormat::Compressed_SRGB_ALPHA_DXT5:
		return GL_RGBA;

	case InternalFormat::InvalidFormat:
	case InternalFormat::KeepParentFormat:
		break;
	}
	return GL_INVALID_ENUM;
}

static uint16_t fmt_channels(Texture::InternalFormat format) noexcept
{
	switch (components(format)) {
	case GL_RED:
		return 1;
	case GL_RG:
		return 2;
	case GL_RGB:
		return 3;
	case GL_RGBA:
		return 4;
	default:
		break;
	}
	return 0;
}


static Texture::ColorSpace fmt_color_space(Texture::InternalFormat format) noexcept
{
	using namespace Texture;
	switch (format) {
	case InternalFormat::SRGB_8:
	case InternalFormat::SRGB_8_ALPHA_8:
	case InternalFormat::Compressed_SRGB_DXT1:
	case InternalFormat::Compressed_SRGB_ALPHA_DXT1:
	case InternalFormat::Compressed_SRGB_ALPHA_DXT3:
	case InternalFormat::Compressed_SRGB_ALPHA_DXT5:
		return ColorSpace::sRGB;

	default:
		break;
	}
	return ColorSpace::Linear;
}

static bool is_internal_format_invalid(Texture::InternalFormat format) noexcept
{
	return format == Texture::InternalFormat::KeepParentFormat
	    || format == Texture::InternalFormat::InvalidFormat;
}

static bool is_internal_format_compressed(Texture::InternalFormat format) noexcept
{
	switch (format) {
	case Texture::InternalFormat::Compressed_RGB_DXT1:
	case Texture::InternalFormat::Compressed_RGBA_DXT1:
	case Texture::InternalFormat::Compressed_RGBA_DXT3:
	case Texture::InternalFormat::Compressed_RGBA_DXT5:
	case Texture::InternalFormat::Compressed_SRGB_DXT1:
	case Texture::InternalFormat::Compressed_SRGB_ALPHA_DXT1:
	case Texture::InternalFormat::Compressed_SRGB_ALPHA_DXT3:
	case Texture::InternalFormat::Compressed_SRGB_ALPHA_DXT5:
		return true;
	default:
		break;
	}
	return false;
}

static Texture::InternalFormat reinterpret_format(Texture::InternalFormat newfmt)
{
	if(newfmt != Texture::InternalFormat::KeepParentFormat) {
		newfmt = Texture::InternalFormat::KeepParentFormat;
		fmt::print(stderr, "[texture2d.storage] format change not yet implemented!");
	}
	return newfmt;
}

static uint16_t max_size;
static uint16_t max_layers;

static void get_max_size()
{
	if(unlikely(!max_size)) {
		GLint max_tex_size{}, max_buffer_size{}, max_layers_gl{};
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
		glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_buffer_size);
		glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers_gl);
		fmt::print("[texture2d.storage] limits:\n"
		           "    Texture size:        {}\n"
		           "    Texture buf:         {}\n"
		           "    Layers:              {}\n",
		           max_tex_size,
		           max_buffer_size,
		           max_layers_gl);
		max_size = max_tex_size;
		max_layers = std::min(max_layers_gl, 1);
	}
}

//------------------------------------------------------------------------------


TextureStorage2D::TextureStorage2D(uint16_t width,
                                   uint16_t height,
                                   Texture::InternalFormat format)
{
	get_max_size();

	if(width <= 2
	   || height <= 2
	   || width > max_size
	   || height > max_size
	   || is_internal_format_invalid(format))
		return;

	m_mip_levels = static_cast<std::uint8_t>(zcm::floor(zcm::log2(std::max(width, height))) + 1);
	m_width = width;
	m_height = height;
	m_internal_format = format;
	glCreateTextures(GL_TEXTURE_2D, 1, m_handle.get());
	if(m_handle)
		glTextureStorage2D(*m_handle, m_mip_levels, static_cast<GLenum>(format), width, height);
}

TextureStorage2D TextureStorage2D::share_view(unsigned minlevel, unsigned numlevels, Texture::InternalFormat view_format)
{
	using namespace Texture;
	TextureStorage2D ret;
	if(!m_handle)
		return ret;


	if(minlevel != 0) {
		minlevel = 0;
		fmt::print(stderr, "[texture2d.storage] base level change not yet implemented");
	}

	if(minlevel + numlevels > m_mip_levels) {
		fmt::print(stderr, "[texture2d.storage] mip levels OOB: base: {} count: {} parent: {}", minlevel, numlevels, m_mip_levels);
		return ret;
	}

	view_format = reinterpret_format(view_format);
	if(view_format == InternalFormat::KeepParentFormat) {
		view_format = m_internal_format;
	}

	ret.m_internal_format = view_format;
	ret.m_mip_levels = numlevels;
	ret.m_width = m_width;
	ret.m_height = m_height;

	auto increment_clamp = [](uint8_t val)
	{
		if(val < std::numeric_limits<uint8_t>::max())
			++val;
		return val;
	};


	glGenTextures(1, ret.m_handle.get()); // must not bind to target here
	if(ret.m_handle) {
		glTextureView(*ret.m_handle, GL_TEXTURE_2D, *m_handle, static_cast<GLenum>(ret.m_internal_format), minlevel, ret.m_mip_levels, 0, 1);
		// if this is first time we share this texture, remember original handle
		m_shared = increment_clamp(m_shared);
		ret.m_shared = m_shared;
	}

	return ret;

}

void TextureStorage2D::sub_image(uint16_t level,
                                  uint16_t width,
                                  uint16_t height,
                                  Texture::TexelDataType type,
                                  const void * pixels)
{
	if(unlikely(!m_handle)) return;
	if(m_shared) {
		fmt::print(stderr, "[texture2d.storage] attempted to mutate shared texture: not implemented\n");
		return;
	}

	auto component_format = components(m_internal_format);
	glTextureSubImage2D(*m_handle, level, 0, 0, width, height, component_format, static_cast<GLenum>(type), pixels);
}

void TextureStorage2D::compressed_sub_image(uint16_t level,
                                            uint16_t width,
                                            uint16_t height,
                                            size_t data_size,
                                            const void *data)
{
	assert(is_internal_format_compressed(m_internal_format));
	if(unlikely(!m_handle)) return;
	if(m_shared) {
		fmt::print(stderr, "[texture2d.storage] attempted to mutate shared texture: not implemented\n");
		return;
	}
	glCompressedTextureSubImage2D(*m_handle, level, 0, 0, width, height, static_cast<GLenum>(m_internal_format), data_size, data);
}

uint16_t TextureStorage2D::channels() const noexcept
{
	return fmt_channels(m_internal_format);
}

Texture::ColorSpace TextureStorage2D::color_space() const noexcept
{
	return fmt_color_space(m_internal_format);
}


//------------------------------------------------------------------------------



ImageTexture2D ImageTexture2D::fromFile(const std::string_view path,
                                        Texture::ColorSpace color_space)
{
	using namespace Texture;


	int width, height, nrChannels;
	auto data = stbi_load(path.data(), &width, &height, &nrChannels, 0);
	if(!data) {
		fmt::print(stderr, "[texture2d.fromFile] could not load image data from [{}]\n", path);
		return ImageTexture2D();
	}

	TextureStorage2D storage;
	SwizzleMask mask;

	switch (nrChannels) {
	case 1:
	{
		storage = TextureStorage2D(width, height, InternalFormat::R_8);
		storage.sub_image(0, width, height, TexelDataType::UnsignedByte, data);

		mask = SwizzleMask{ChannelValue::Red, ChannelValue::Red, ChannelValue::Red, ChannelValue::One};
		break;
	}
	case 3:
	{
		auto format = color_space == ColorSpace::Linear ? InternalFormat::RGB_8 : InternalFormat::SRGB_8;
		storage = TextureStorage2D(width, height, format);
		storage.sub_image(0, width, height, TexelDataType::UnsignedByte, data);
		break;
	}
	case 4:
	{
		auto format = color_space == ColorSpace::Linear ? InternalFormat::RGBA_8 : InternalFormat::SRGB_8_ALPHA_8;
		storage = TextureStorage2D(width, height, format);
		storage.sub_image(0, width, height, TexelDataType::UnsignedByte, data);
		break;
	}
	default:
		unreachable();
	}

	ImageTexture2D ret;
	ret.m_storage = std::move(storage);
	ret.set_swizzle_mask(mask);

	stbi_image_free(data);
	if(ret.m_storage.valid()) {
		glGenerateTextureMipmap(ret.m_storage.texture_handle());
		ret.set_default_params();
	}
	return ret;
}


ImageTexture2D ImageTexture2D::share()
{
	ImageTexture2D copy;
	copy.m_anisotropic_samples = m_anisotropic_samples;
	copy.m_border_color = m_border_color;
	copy.m_filtering = m_filtering;
	copy.m_mipmapping = m_mipmapping;
	copy.m_swizzle_mask = m_swizzle_mask;
	copy.m_wrapping = m_wrapping;
	copy.m_storage = m_storage.share();
	copy.set_default_params();
	return copy;
}


bool ImageTexture2D::bind_to_unit(uint32_t unit) const noexcept
{
	if(!valid()) return false;
	glBindTextureUnit(unit, m_storage.texture_handle());
	return true;
}

static void set_min_filter(GLuint tex, Texture::MipMapping m, Texture::Filtering f)
{
	GLenum filter{};
	switch (m) {
	case Texture::MipMapping::Disable:
		switch (f) {
		case Texture::Filtering::Linear:
			filter = GL_LINEAR;
			break;
		case Texture::Filtering::Nearest:
			filter = GL_NEAREST;
			break;
		}
		break;

	case Texture::MipMapping::MipLinear:
		switch (f) {
		case Texture::Filtering::Linear:
			filter = GL_LINEAR_MIPMAP_LINEAR;
			break;
		case Texture::Filtering::Nearest:
			filter = GL_NEAREST_MIPMAP_LINEAR;
			break;
		}
		break;
	case Texture::MipMapping::MipNearest:
		switch (f) {
		case Texture::Filtering::Linear:
			filter = GL_LINEAR_MIPMAP_NEAREST;
			break;
		case Texture::Filtering::Nearest:
			filter = GL_NEAREST_MIPMAP_NEAREST;
			break;
		}
		break;
	}
	assert(filter != 0 && "invalid texture filtering");

	glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, filter);
}


void ImageTexture2D::set_filtering(Texture::MipMapping m, Texture::Filtering f) noexcept
{
	m_mipmapping = m;
	m_filtering = f;
	if(unlikely(!m_storage.valid())) return;
	set_min_filter(m_storage.texture_handle(), m_mipmapping, m_filtering);

	GLenum mag_filtering{GL_INVALID_ENUM};
	switch (m_filtering) {
	case Texture::Filtering::Linear:
		mag_filtering = GL_LINEAR;
		break;

	case Texture::Filtering::Nearest:
		mag_filtering = GL_NEAREST;
		break;
	}
	assert(mag_filtering != GL_INVALID_ENUM);
	glTextureParameteri(m_storage.texture_handle(), GL_TEXTURE_MAG_FILTER, mag_filtering);
}


void ImageTexture2D::set_wrapping(Texture::Wrapping w) noexcept
{
	m_wrapping = w;
	if(unlikely(!m_storage.valid())) return;
	GLenum wrapping{GL_INVALID_ENUM};

	auto get_wrapping = [](Texture::Wrapping w)
	{
		static const GLenum apival[]
		{
			GL_CLAMP_TO_BORDER,
			GL_CLAMP_TO_EDGE,
			GL_MIRROR_CLAMP_TO_EDGE,
			GL_REPEAT,
			GL_MIRRORED_REPEAT
		};

		return apival[static_cast<uint8_t>(w)];
	};

	wrapping = get_wrapping(w);

	assert(wrapping != GL_INVALID_ENUM);

	glTextureParameteri(m_storage.texture_handle(), GL_TEXTURE_WRAP_S, wrapping);
	glTextureParameteri(m_storage.texture_handle(), GL_TEXTURE_WRAP_R, wrapping);
}


void ImageTexture2D::set_anisotropy(unsigned samples) noexcept
{
	static unsigned max_aniso;
	if(unlikely(!max_aniso)) {
		float max_aniso_samples = 0.0f;
		constexpr auto GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT = gl45ext::GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso_samples);

		max_aniso = zcm::floor(max_aniso_samples);
		if(max_aniso < 2 || max_aniso > 16) {
			fmt::print(stderr, "[texture2d] driver reports invalid maximum anisotropy sample count: {}\n", max_aniso_samples);
			max_aniso = 1;
		} else {
			//fmt::print(stderr, "[texture2d.filtering] max anisotropic samples: {}\n", max_aniso);
		}
	}
	m_anisotropic_samples = std::min(std::max(samples, 1u), max_aniso);
	if(unlikely(!m_storage.valid())) return;

	constexpr auto GL_TEXTURE_MAX_ANISOTROPY_EXT = gl45ext::GL_TEXTURE_MAX_ANISOTROPY_EXT;
	float aniso = m_anisotropic_samples;
	glTextureParameterf(m_storage.texture_handle(), GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
}


void ImageTexture2D::set_border_color(const zcm::vec4& c) noexcept
{
	m_border_color = c;
	if(unlikely(!m_storage.valid())) return;

	glTextureParameterfv(m_storage.texture_handle(), GL_TEXTURE_BORDER_COLOR, zcm::value_ptr(m_border_color));
}


void ImageTexture2D::set_swizzle_mask(const Texture::SwizzleMask& m) noexcept
{
	m_swizzle_mask = m;
	if(unlikely(!m_storage.valid())) return;

	auto repr = [](Texture::ChannelValue v)
	{
		static const GLenum apival[] =
		{
			GL_ZERO,
			GL_ONE,
			GL_RED,
			GL_GREEN,
			GL_BLUE,
			GL_ALPHA,
		};
		return apival[static_cast<uint8_t>(v)];
	};

	const GLenum swizzleMask[] = {repr(m.red), repr(m.green), repr(m.blue), repr(m.alpha)};
	glTextureParameteriv(m_storage.texture_handle(), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
}

void ImageTexture2D::set_default_params()
{
	if(unlikely(!valid())) return;

	set_filtering(m_mipmapping, m_filtering);
	set_wrapping(m_wrapping);
	set_anisotropy(m_anisotropic_samples);
	set_border_color(m_border_color);
	set_swizzle_mask(m_swizzle_mask);
}


