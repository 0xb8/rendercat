#pragma once
#include <rendercat/shaders/common.h>
#include <cstdint>


namespace rc {
namespace Texture {

	enum class AlphaMode : std::uint8_t {
		Unknown = 0,
		Opaque,
		Mask,
		Blend,
	};

	enum class Kind : std::uint16_t
	{
		None      = 0,
		BaseColor = RC_SHADER_TEXTURE_KIND_BASE_COLOR,
		Normal    = RC_SHADER_TEXTURE_KIND_NORMAL,
		RoughnessMetallic = RC_SHADER_TEXTURE_KIND_ROUGNESS_METALLIC,
		Occlusion = RC_SHADER_TEXTURE_KIND_OCCLUSION,
		OcclusionRoughnessMetallic = Occlusion | RoughnessMetallic,
		OcclusionSeparate = Occlusion | RC_SHADER_TEXTURE_SEPARATE_OCCLUSION,
		Emission  = RC_SHADER_TEXTURE_KIND_EMISSION,
	};

	enum class ColorSpace : std::int8_t
	{
		Linear,
		sRGB
	};

	enum class InternalFormat : std::uint16_t
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
		Compressed_SRGB_ALPHA_DXT5 = 0x8C4F,

		// -- RGTC
		Compressed_RED_RGTC1 = 0x8DBB,
		Compressed_SIGNED_RED_RGTC1 = 0x8DBC,
		Compressed_RG_RGTC2 = 0x8DBD,
		Compressed_SIGNED_RG_RGTC2 = 0x8DBE,
		// -- BPTC (BC7)
		Compressed_RGBA_BPTC_UNORM = 0x8E8C,
		Compressed_SRGB_ALPHA_BPTC_UNORM = 0x8E8D,
		Compressed_RGB_BPTC_SIGNED_FLOAT = 0x8E8E,
		Compressed_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F,
	};

	enum class TexelDataType : std::uint16_t
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

	enum class MagFilter : uint16_t
	{
		Nearest = 9728,
		Linear = 9729
	};

	enum class MinFilter : uint16_t
	{
		Nearest = 9728,
		Linear = 9729,
		NearestMipMapNearest = 9984,
		LinearMipMapNearest = 9985,
		NearestMipMapLinear = 9986,
		LinearMipMapLinear = 9987
	};

	enum class Wrapping : std::uint16_t
	{
		Repeat = 0x2901,
		MirroredRepeat = 0x8370,
		ClampToBorder = 0x812D,
		ClampToEdge = 0x812F,
		MirrorClampToEdge = 0x8743
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

	// FIXME: use X-macros or some other reflection tool
	const char* enum_value_str(AlphaMode) noexcept;
	const char* enum_value_str(ColorSpace) noexcept;
	const char* enum_value_str(ChannelValue) noexcept;
	const char* enum_value_str(MagFilter) noexcept;
	const char* enum_value_str(MinFilter) noexcept;
	const char* enum_value_str(InternalFormat) noexcept;
	const char* enum_value_str(Kind) noexcept;
	const char* enum_value_str(Wrapping) noexcept;

} // namespace Texture
} // namespace rc
