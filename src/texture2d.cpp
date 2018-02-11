#include <rendercat/texture2d.hpp>
#include <stb_image.h>
#include <fmt/format.h>

#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding/gl45ext/enum.h>

using namespace gl45core;
using namespace rc;

ImageTextureStorage2D::ImageTextureStorage2D(uint16_t width,
                                             uint16_t height,
                                             Texture::InternalFormat format)
{
	static uint16_t max_size;
	if(unlikely(!max_size)) {
		GLint max_tex_size{}, max_buffer_size{};
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
		glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_buffer_size);
		fmt::print("[texture2d.storage] limits:\n"
		           "    Texture size:        {}\n"
		           "    Texture buf:         {}\n",
		           max_tex_size,
		           max_buffer_size);
		max_size = max_tex_size;
	}

	if(width <= 2 || height <= 2 || width > max_size || height > max_size)
		return;

	m_mip_levels = (uint8_t)std::floor(std::log2(std::max(width, height))) + 1;
	m_width = width;
	m_height = height;
	m_internal_format = format;
	glCreateTextures(GL_TEXTURE_2D, 1, m_handle.get());
	if(m_handle)
		glTextureStorage2D(*m_handle, m_mip_levels, static_cast<GLenum>(format), width, height);
}

ImageTextureStorage2D ImageTextureStorage2D::share_view(unsigned minlevel, unsigned numlevels, Texture::InternalFormat view_format)
{
	using namespace Texture;
	ImageTextureStorage2D ret;
	if(!m_handle)
		return ret;

	if(view_format != InternalFormat::KeepParentFormat) {
		view_format = InternalFormat::KeepParentFormat;
		fmt::print(stderr, "[texture2d.storage] format change not yet implemented!");
	}
	if(minlevel != 0) {
		minlevel = 0;
		fmt::print(stderr, "[texture2d.storage] base level change not yet implemented");
	}

	if(minlevel + numlevels > m_mip_levels) {
		fmt::print(stderr, "[texture2d.storage] mip levels OOB: base: {} count: {} parent: {}", minlevel, numlevels, m_mip_levels);
		return ret;
	}

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

void ImageTextureStorage2D::subImage2D(uint16_t level,
                                       uint16_t width,
                                       uint16_t height,
                                       Texture::PixelDataType type,
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

void ImageTextureStorage2D::compressedSubImage2D(uint16_t, uint16_t, uint16_t, size_t, const void *)
{
	// not implemented
}

uint16_t ImageTextureStorage2D::channels() const noexcept
{
	switch (components(m_internal_format)) {
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

Texture::ColorSpace ImageTextureStorage2D::color_space() const noexcept
{
	using namespace Texture;
	switch (m_internal_format) {
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

ImageTexture2D ImageTexture2D::fromFile(const std::string_view path,
                                        Texture::ColorSpace color_space)
{
	using namespace Texture;
	ImageTexture2D ret{};
	int width, height, nrChannels;
	auto data = stbi_load(path.data(), &width, &height, &nrChannels, 0);
	if(!data) {
		fmt::print(stderr, "[texture2d.fromFile] could not load image data from [{}]\n", path);
		return ret;
	}

	switch (nrChannels) {
	case 1:
	{

		ImageTextureStorage2D storage(width, height, InternalFormat::R_8);
		storage.subImage2D(0, width, height, PixelDataType::UnsignedByte, data);
		ret.m_storage = std::move(storage);
		ret.setSwizzleMask(SwizzleMask{ChannelValue::Red, ChannelValue::Red, ChannelValue::Red, ChannelValue::One});
		break;
	}
	case 3:
	{
		auto format = color_space == ColorSpace::Linear ? InternalFormat::RGB_8 : InternalFormat::SRGB_8;
		ImageTextureStorage2D storage(width, height, format);
		storage.subImage2D(0, width, height, PixelDataType::UnsignedByte, data);
		ret.m_storage = std::move(storage);
		break;
	}
	case 4:
	{
		auto format = color_space == ColorSpace::Linear ? InternalFormat::RGBA_8 : InternalFormat::SRGB_8_ALPHA_8;
		ImageTextureStorage2D storage(width, height, format);
		storage.subImage2D(0, width, height, PixelDataType::UnsignedByte, data);
		ret.m_storage = std::move(storage);
		break;
	}
	default:
		return ret;
	}

	stbi_image_free(data);
	if(ret.m_storage.valid()) {
		glGenerateTextureMipmap(ret.m_storage.texture_handle());
		ret.setAllParams();
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
	copy.setAllParams();
	return copy;
}

bool ImageTexture2D::bindToUnit(uint32_t unit) const noexcept
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

void ImageTexture2D::setFiltering(Texture::MipMapping m, Texture::Filtering f) noexcept
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

void ImageTexture2D::setWrapping(Texture::Wrapping w) noexcept
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

void ImageTexture2D::setAnisotropy(unsigned samples) noexcept
{
	static unsigned max_aniso;
	if(unlikely(!max_aniso)) {
		float max_aniso_samples = 0.0f;
		constexpr auto GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT = gl45ext::GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso_samples);

		max_aniso = std::floor(max_aniso_samples);
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

void ImageTexture2D::setBorderColor(const glm::vec4& c) noexcept
{
	m_border_color = c;
	if(unlikely(!m_storage.valid())) return;

	glTextureParameterfv(m_storage.texture_handle(), GL_TEXTURE_BORDER_COLOR, glm::value_ptr(m_border_color));
}

void ImageTexture2D::setSwizzleMask(const Texture::SwizzleMask& m) noexcept
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

void ImageTexture2D::setAllParams()
{
	if(unlikely(!valid())) return;

	setFiltering(m_mipmapping, m_filtering);
	setWrapping(m_wrapping);
	setAnisotropy(m_anisotropic_samples);
	setBorderColor(m_border_color);
	setSwizzleMask(m_swizzle_mask);
}


// ----------------------------------------------------------------------------

const char* Texture::enum_value_str(Texture::ColorSpace s) noexcept
{
	using namespace Texture;
	switch (s) {
	case ColorSpace::sRGB:
		return "sRGB";
	case ColorSpace::Linear:
		return "Linear";
	}
	unreachable();
}

const char* Texture::enum_value_str(Texture::InternalFormat f) noexcept
{
	using namespace Texture;
	switch (f) {
	case InternalFormat::R_8:
		return "R_8";
	case InternalFormat::RGB_8:
		return "RGB_8";
	case InternalFormat::SRGB_8:
		return "SRGB_8";
	case InternalFormat::RGBA_8:
		return "RGBA_8";
	case InternalFormat::SRGB_8_ALPHA_8:
		return "SRGB_8_ALPHA_8";
	default:
		break;
	}
	return "Some other"; // TODO: fill up other formats
}

const char* Texture::enum_value_str(Texture::MipMapping m) noexcept
{
	using namespace Texture;
	switch (m) {
	case MipMapping::Disable:
		return "Disabled";
	case MipMapping::MipLinear:
		return "MipLinear";
	case MipMapping::MipNearest:
		return "MipNearest";
	}
	unreachable();
}

const char* Texture::enum_value_str(Texture::Filtering f) noexcept
{
	using namespace Texture;
	switch (f) {
	case Filtering::Nearest:
		return "Nearest";
	case Filtering::Linear:
		return "Linear";
	}
	unreachable();
}

const char* Texture::enum_value_str(Texture::Wrapping w) noexcept
{
	using namespace Texture;
	switch (w) {
	case Wrapping::ClampToBorder:
		return "ClampToBorder";
	case Wrapping::ClampToEdge:
		return "ClampToEdge";
	case Wrapping::ClampToEdgeMirror:
		return "ClampToEdgeMirror";
	case Wrapping::Repeat:
		return "Repeat";
	case Wrapping::RepeatMirror:
		return "RepeatMirror";
	}
	unreachable();
}

const char* Texture::enum_value_str(Texture::ChannelValue c) noexcept
{
	using namespace Texture;
	switch (c) {
	case ChannelValue::Zero:
		return "Zero";
	case ChannelValue::One:
		return "One";
	case ChannelValue::Red:
		return "Red";
	case ChannelValue::Green:
		return "Green";
	case ChannelValue::Blue:
		return "Blue";
	case ChannelValue::Alpha:
		return "Alpha";
	}
	unreachable();
}

const char* Texture::enum_value_str(Texture::AlphaMode mode) noexcept
{
	using namespace Texture;
	switch (mode) {
	case AlphaMode::Opaque:
		return "Opaque";
	case AlphaMode::Blend:
		return "Blend";
	case AlphaMode::Mask:
		return "Mask";
	case AlphaMode::Unknown:
		return "Unknown";
	}
	unreachable();
}

const char* Texture::enum_value_str(Texture::Kind map) noexcept
{
	using namespace Texture;
	switch (map) {
	case Kind::Diffuse:
		return "Diffuse";
	case Kind::Emission:
		return "Emission";
	case Kind::Metallic:
		return "Metallic";
	case Kind::Normal:
		return "Normal";
	case Kind::Occlusion:
		return "Occlusion";
	case Kind::Roughness:
		return "Roughness";
	case Kind::Specular:
		return "Specular";
	case Kind::None:
		return "None";
	}
	unreachable();
}
