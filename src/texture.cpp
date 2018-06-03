#include <rendercat/texture.hpp>
#include <rendercat/common.hpp>

using namespace rc::Texture;

const char* rc::Texture::enum_value_str(ColorSpace s) noexcept
{
	switch (s) {
	case ColorSpace::sRGB:
		return "sRGB";
	case ColorSpace::Linear:
		return "Linear";
	}
	unreachable();
}

const char* rc::Texture::enum_value_str(InternalFormat f) noexcept
{
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

const char* rc::Texture::enum_value_str(MipMapping m) noexcept
{
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

const char* rc::Texture::enum_value_str(Filtering f) noexcept
{
	switch (f) {
	case Filtering::Nearest:
		return "Nearest";
	case Filtering::Linear:
		return "Linear";
	}
	unreachable();
}

const char* rc::Texture::enum_value_str(Wrapping w) noexcept
{
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

const char* rc::Texture::enum_value_str(ChannelValue c) noexcept
{
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

const char* rc::Texture::enum_value_str(AlphaMode mode) noexcept
{
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

const char* rc::Texture::enum_value_str(Kind map) noexcept
{
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