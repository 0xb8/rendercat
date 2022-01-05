#pragma once
#include <rendercat/common.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <filesystem>
#include <zcm/fwd.hpp>

namespace rc {

class ShaderSet;
class Cubemap;

namespace Texture {
	bool bind_to_unit(const Cubemap& cubemap, uint32_t unit) noexcept;
}

class Cubemap
{
public:

	Cubemap();
	~Cubemap() = default;

	RC_DEFAULT_MOVE_NOEXCEPT(Cubemap)
	RC_DISABLE_COPY(Cubemap)

	void load_cube(const std::filesystem::path& dir);
	void load_equirectangular(const std::filesystem::path& file);

	[[nodiscard]] static Cubemap integrate_diffuse_irradiance(const Cubemap& source);
	[[nodiscard]] static Cubemap convolve_specular(const Cubemap& source);

	static void compile_shaders(ShaderSet& shader_set);
	static void draw(const Cubemap& cubemap, const zcm::mat4& view, const zcm::mat4& projection, int mip_level = 0) noexcept;

private:
	friend bool Texture::bind_to_unit(const Cubemap& cubemap, uint32_t unit) noexcept;
	texture_handle m_cubemap;
};



} // namespace rc
