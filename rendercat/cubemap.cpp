#include <rendercat/cubemap.hpp>
#include <rendercat/uniform.hpp>
#include <rendercat/shader_set.hpp>
#include <rendercat/util/gl_debug.hpp>
#include <stb_image.h>
#include <string>
#include <utility>
#include <cassert>
#include <filesystem>
#include <fmt/core.h>

#include <zcm/mat4.hpp>
#include <zcm/mat3.hpp>
#include <zcm/matrix_transform.hpp>
#include <zcm/common.hpp>
#include <zcm/angle_and_trigonometry.hpp>

#include <glbinding/gl/extension.h>
#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding-aux/Meta.h>
#include <rendercat/util/gl_meta.hpp>

#include <tracy/Tracy.hpp>
using namespace gl45core;
#include <tracy/TracyOpenGL.hpp>
using namespace rc;

static const std::array<std::filesystem::path, 6> face_names_hdr {
	"right.hdr",
	"left.hdr",
	"top.hdr",
	"bottom.hdr",
	"front.hdr",
	"back.hdr"
};

static GLint max_texture_size;                       // spec requires 16384+, but some cards can only 8192
static GLboolean has_seamless_filtering;             // GL_ARB_seamless_cube_map is widely supported
static GLboolean has_seamless_filtering_per_texture; // requires GL_ARB_seamless_cubemap_per_texture, checked below
static GLuint cubemap_vao;
static uint32_t *cubemap_draw_shader;
static uint32_t *cubemap_load_shader;
static uint32_t *compute_diffuse_irradiance_shader;
static uint32_t *compute_specular_env_map_shader;
static uint32_t *minimal_atmosphere_shader;
static uint32_t *compute_minimal_atmosphere_bake_shader;

static void set_tex_params(uint32_t tex, bool mips=false)
{
	glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (has_seamless_filtering_per_texture)
		glTextureParameteri(tex, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
}



Cubemap::Cubemap()
{
	if(!max_texture_size) {
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_texture_size);
		has_seamless_filtering = glIsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		has_seamless_filtering_per_texture = rc::glmeta::extension_supported(gl::GLextension::GL_ARB_seamless_cubemap_per_texture);
		if (!has_seamless_filtering_per_texture)
			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#ifndef NDEBUG
		fmt::print(stderr,
		           "[cubemap] limits:\n"
		           "    Texture size:        {}\n"
		           "    Seamless filtering:\n"
		           "        Global:          {}\n"
		           "        Per-texture:     {}\n",
		           max_texture_size,
		           glbinding::aux::Meta::getString(has_seamless_filtering),
		           glbinding::aux::Meta::getString(has_seamless_filtering_per_texture));
#endif
	}
	if (!cubemap_vao) {
		glCreateVertexArrays(1, &cubemap_vao);
		rcObjectLabel(GL_VERTEX_ARRAY, cubemap_vao, "cubemap vao");
		assert(cubemap_vao);
	}
}

void Cubemap::load_cube(const std::filesystem::path& dir)
{
	ZoneScoped;

	std::filesystem::path path{dir};
	assert(std::filesystem::is_directory(path));

	rc::texture_handle tex;
	// BUG: AMD driver cannot upload faces of single cubemap, only cubemap array (size of 1 still works)
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, tex.get());
	set_tex_params(*tex);

	bool texture_storage_allocated{false};
	int face{0};
	int prev_face_width{0}, prev_face_height{0};
	for(int i = 0; i < 6; ++i) {
		ZoneScopedN("load face");
		auto face_path = path / face_names_hdr[i];
		auto face_path_u8 = face_path.u8string();

		int face_width, face_height, chan;
		auto data = stbi_loadf(face_path_u8.c_str(), &face_width, &face_height, &chan, 3);
		if(data && chan == 3) {
			if(!texture_storage_allocated) {
				glTextureStorage3D(*tex, 1, GL_RGB16F, face_width, face_height, 6);
				texture_storage_allocated = true;
				prev_face_width = face_width;
				prev_face_height = face_height;
			} else {
				if(face_width != prev_face_width || face_height != prev_face_height) {
					fmt::print(stderr, "[cubemap] bad face '{}': "
					           "face size does not match (current {}x{}, previous {}x{})\n",
					           face_path_u8,
					           face_width,
					           face_height,
					           prev_face_width,
					           prev_face_height);

					stbi_image_free(data);
					return;
				}
			}

			glTextureSubImage3D(*tex, 0, 0, 0, face, face_width, face_height, 1, GL_RGB, GL_FLOAT, data);
			++face;

			stbi_image_free(data);
		} else {
			fmt::print(stderr, "[cubemap] could not open cubemap face '{}'\n", face_path_u8);
			if(chan != 3) {
				fmt::print(stderr, "[cubemap] invalid channel count: {}\n", chan);
			}
			return;
		}
	}
	rcObjectLabel(tex, fmt::format("cubemap: {} ({}x{})", dir.u8string(), prev_face_width, prev_face_height));
	m_cubemap = std::move(tex);
}

void Cubemap::load_equirectangular(const std::filesystem::path& file)
{
	assert(cubemap_load_shader);
	ZoneScoped;
	rc::texture_handle flat_texture;
	int flat_width, flat_height, chan;
	auto data = stbi_loadf(file.u8string().data(), &flat_width, &flat_height, &chan, 3);
	if(data && chan == 3) {
			glCreateTextures(GL_TEXTURE_2D, 1, flat_texture.get());
			glTextureStorage2D(*flat_texture, 1, GL_RGB32F, flat_width, flat_height);
			glTextureSubImage2D(*flat_texture, 0, 0, 0, flat_width, flat_height, GL_RGB, GL_FLOAT, data);
			stbi_image_free(data);
	} else {
		fmt::print(stderr, "[cubemap] could not open equirectangular map file '{}'\n", file.u8string());
		stbi_image_free(data);
		return;
	}

	const unsigned face_size = (flat_height * 3) / 4;

	TracyGpuZone("cubemap_load_equirectangular");
	rc::texture_handle cubemap_to;
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, cubemap_to.get());
	glTextureStorage3D(*cubemap_to, 1, GL_RGBA16F, face_size, face_size, 6);
	set_tex_params(*cubemap_to);

	glUseProgram(*cubemap_load_shader);
	glBindTextureUnit(0, *flat_texture);
	glBindImageTexture(0, *cubemap_to, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(face_size/32, face_size/32, 6);

	m_cubemap = std::move(cubemap_to);
	rcObjectLabel(m_cubemap, fmt::format("cubemap: {} ({}x{})", file.u8string(), face_size, face_size));
}

void Cubemap::draw(const Cubemap & cubemap, const zcm::mat4 & view, const zcm::mat4 & projection, int mip_level) noexcept
{
	assert(cubemap_draw_shader);
	if (Texture::bind_to_unit(cubemap, 0)) {
		// remove translation part from view matrix
		unif::m4(*cubemap_draw_shader, 0, projection * zcm::mat4{zcm::mat3{view}});
		unif::i1(*cubemap_draw_shader, 1, mip_level);

		glUseProgram(*cubemap_draw_shader);
		glBindVertexArray(cubemap_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 14); // see cubemap.vert
		glBindVertexArray(0);
	} else {
		fmt::print(stderr, "[cubemap] attempted to draw invalid cubemap!\n");
		std::fflush(stderr);
	}
}

void Cubemap::draw_atmosphere(const zcm::mat4 & view, const zcm::mat4 & projection, bool draw_planet) noexcept
{
	assert(minimal_atmosphere_shader);
	// remove translation part from view matrix
	unif::m4(*minimal_atmosphere_shader, 0, projection * zcm::mat4{zcm::mat3{view}});
	unif::b1(*minimal_atmosphere_shader, 1, draw_planet);

	glUseProgram(*minimal_atmosphere_shader);
	glBindVertexArray(cubemap_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 14); // see cubemap.vert
	glBindVertexArray(0);
}

void Cubemap::draw_atmosphere_to_cube(Cubemap& cube, int size, bool draw_planet) noexcept
{
	assert(compute_minimal_atmosphere_bake_shader);
	if (!cube.m_cubemap) {
		rc::texture_handle tex;
		// BUG: AMD driver cannot upload faces of single cubemap, only cubemap array (size of 1 still works)
		glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, tex.get());
		set_tex_params(*tex);

		// create texture array for point light shadowmaps
		glTextureStorage3D(*tex, 1, GL_RGBA16F, size, size, 6);
		rcObjectLabel(tex, fmt::format("Baked atmosphere cubemap ({}x{})", size, size));
		cube.m_cubemap = std::move(tex);
	}
	unif::b1(*compute_minimal_atmosphere_bake_shader, 1, draw_planet);
	glUseProgram(*compute_minimal_atmosphere_bake_shader);
	glBindImageTexture(0, *cube.m_cubemap, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(size/16, size/16, 6);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT);
}


Cubemap Cubemap::integrate_diffuse_irradiance(const Cubemap & source)
{
	assert(compute_diffuse_irradiance_shader);
	ZoneScoped;
	TracyGpuZone("cubemap_integrate_diffuse_irradiance");
	Cubemap res;
	if (source.m_cubemap) {
		const unsigned irradiance_size = 32;
		rc::texture_handle irradiance_cube_to;
		glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, irradiance_cube_to.get());
		glTextureStorage3D(*irradiance_cube_to, 1, GL_RGBA16F, irradiance_size, irradiance_size, 6);
		set_tex_params(*irradiance_cube_to);

		glUseProgram(*compute_diffuse_irradiance_shader);
		glBindTextureUnit(0, *source.m_cubemap);
		glBindImageTexture(0, *irradiance_cube_to, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glDispatchCompute(irradiance_size/16, irradiance_size/16, 6);

		res.m_cubemap = std::move(irradiance_cube_to);
	}
	return res;
}

Cubemap Cubemap::convolve_specular(const Cubemap & source)
{
	assert(compute_specular_env_map_shader);
	Cubemap res;
	if (!source.m_cubemap)
		return res;

	ZoneScoped;
	TracyGpuZone("cubemap_convolve_specular");

	rc::texture_handle cubemap_with_mips_to;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, cubemap_with_mips_to.get());

	GLint src_size, dst_size;
	glGetTextureLevelParameteriv(*source.m_cubemap, 0, GL_TEXTURE_WIDTH, &src_size);
	dst_size = std::min(std::max(src_size, 64), 256);
	int dst_numlevels = rc::math::num_mipmap_levels(dst_size, dst_size);
	glTextureStorage2D(*cubemap_with_mips_to, dst_numlevels, GL_RGBA16F, dst_size, dst_size);
	set_tex_params(*cubemap_with_mips_to, true);


	{
		ZoneScopedN("downsample");
		// downsample original cubemap
		rc::framebuffer_handle src_fbo, dst_fbo;
		glCreateFramebuffers(1, src_fbo.get());
		glCreateFramebuffers(1, dst_fbo.get());

		for (int i = 0; i < 6; ++i) {
			glNamedFramebufferTextureLayer(*src_fbo, GL_COLOR_ATTACHMENT0, *source.m_cubemap, 0, i);
			glNamedFramebufferTextureLayer(*dst_fbo, GL_COLOR_ATTACHMENT0, *cubemap_with_mips_to, 0, i);
			glBlitNamedFramebuffer(*src_fbo, *dst_fbo,
					       0, 0, src_size, src_size,
					       0, 0, dst_size, dst_size,
					       GL_COLOR_BUFFER_BIT,
					       GL_LINEAR);
		}
		glGenerateTextureMipmap(*cubemap_with_mips_to);
	}

	rc::texture_handle specular_cube_to;
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, specular_cube_to.get());
	glTextureStorage3D(*specular_cube_to, dst_numlevels, GL_RGBA16F, dst_size, dst_size, 6);

	// Copy 0th mipmap level into destination environment map.
	glCopyImageSubData(*cubemap_with_mips_to, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
	                   *specular_cube_to,     GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0,
	                   dst_size, dst_size, 6);

	glUseProgram(*compute_specular_env_map_shader);
	glBindTextureUnit(0, *cubemap_with_mips_to);

	// Pre-filter rest of the mip chain.
	const float deltaRoughness = 1.0f / zcm::max(float(dst_numlevels-1), 1.0f);
	for(int level=1; level <= dst_numlevels; ++level, dst_size/=2) {
		const GLuint numGroups = std::max(1, dst_size/32);
		glBindImageTexture(0, *specular_cube_to, level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

		unif::f1(*compute_specular_env_map_shader, 0, level * deltaRoughness);
		glDispatchCompute(numGroups, numGroups, 6);
	}
	set_tex_params(*specular_cube_to, true);


	res.m_cubemap = std::move(specular_cube_to);
	return res;
}

void Cubemap::compile_shaders(ShaderSet& shader_set)
{
	cubemap_draw_shader = shader_set.load_program({"cubemap.vert", "cubemap.frag"});
	cubemap_load_shader = shader_set.load_program({"cubemap_from_equirectangular.comp"});
	compute_diffuse_irradiance_shader = shader_set.load_program({"cubemap_diffuse_irradiance.comp"});
	compute_specular_env_map_shader = shader_set.load_program({"cubemap_specular_envmap.comp"});
	minimal_atmosphere_shader = shader_set.load_program({"cubemap.vert", "cubemap_atmosphere.frag"});
	compute_minimal_atmosphere_bake_shader = shader_set.load_program({"cubemap_bake.comp"});
}

bool Texture::bind_to_unit(const Cubemap & cubemap, uint32_t unit) noexcept
{
	if (!cubemap.m_cubemap)
		return false;

	glBindTextureUnit(unit, *cubemap.m_cubemap);
	return true;
}
