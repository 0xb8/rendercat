#pragma once
#include <rendercat/common.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <string_view>
#include <zcm/fwd.hpp>

namespace rc {

class Cubemap
{
public:

	Cubemap();
	~Cubemap() = default;

	RC_DEFAULT_MOVE_NOEXCEPT(Cubemap)
	RC_DISABLE_COPY(Cubemap)

	void load_cube(std::string_view basedir);
	void load_equirectangular(std::string_view path);
	Cubemap integrate_diffuse_irradiance();
	Cubemap convolve_specular();

	void draw(const zcm::mat4& view, const zcm::mat4& projection, int mip_level = 0) noexcept;
	void bind_to_unit(uint32_t unit);

private:
	texture_handle m_cubemap;
};

} // namespace rc
