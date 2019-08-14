#include <rendercat/cubemap.hpp>
#include <rendercat/uniform.hpp>
#include <rendercat/shader_set.hpp>
#include <rendercat/util/gl_perfquery.hpp>
#include <stb_image.h>
#include <string>
#include <utility>
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

using namespace gl45core;
using namespace rc;

static const char* const face_names_hdr[6] = {
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
static rc::ShaderSet shader_set;
static GLuint cubemap_vao;
static uint32_t *cubemap_draw_shader;
static uint32_t *cubemap_load_shader;
static uint32_t *compute_diffuse_irradiance_shader;
static uint32_t *compute_specular_env_map_shader;

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
		assert(cubemap_vao);
	}
	if (!cubemap_draw_shader) {
		cubemap_draw_shader = shader_set.load_program({"cubemap.vert", "cubemap.frag"});
		assert(cubemap_draw_shader);
	}
	if (!cubemap_load_shader) {
		cubemap_load_shader = shader_set.load_program({"cubemap_from_equirectangular.comp"});
		assert(cubemap_load_shader);
	}
	if (!compute_diffuse_irradiance_shader) {
		compute_diffuse_irradiance_shader = shader_set.load_program({"cubemap_diffuse_irradiance.comp"});
		assert(compute_diffuse_irradiance_shader);
	}
	if (!compute_specular_env_map_shader) {
		compute_specular_env_map_shader = shader_set.load_program({"cubemap_specular_envmap.comp"});
	}
}

void Cubemap::load_cube(std::string_view basedir)
{
	assert(basedir.length() > 0);
	std::string path;
	path.reserve(basedir.size() + 20);
	path.append(basedir);
	std::replace(path.begin(), path.end(),'\\', '/');
	if(path.back() != '/') {
		path.push_back('/');
	}
	size_t dirlen = path.length();

	rc::texture_handle tex;
	// BUG: AMD driver cannot upload faces of single cubemap, only cubemap array (size of 1 still works)
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, tex.get());
	glTextureParameteri(*tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(*tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(*tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	if(has_seamless_filtering_per_texture) {
		glTextureParameteri(*tex, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
	}

	bool texture_storage_allocated{false};
	int face{0};
	int prev_face_width{0}, prev_face_height{0};
	for(int i = 0; i < 6; ++i) {
		path.resize(dirlen);
		path.append(face_names_hdr[i]);

		int face_width, face_height, chan;
		auto data = stbi_loadf(path.data(), &face_width, &face_height, &chan, 3);
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
					           path,
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
			fmt::print(stderr, "[cubemap] could not open cubemap face '{}'\n", path);
			if(chan != 3) {
				fmt::print(stderr, "[cubemap] invalid channel count: {}\n", chan);
			}
			return;
		}
	}
	m_cubemap = std::move(tex);
}

void Cubemap::draw(const zcm::mat4 & view, const zcm::mat4 & projection) noexcept
{
	// remove translation part from view matrix
	unif::m4(*cubemap_draw_shader, 0, projection * zcm::mat4{zcm::mat3{view}});

	glUseProgram(*cubemap_draw_shader);
	glBindVertexArray(cubemap_vao);
	glBindTextureUnit(0, *m_cubemap);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 14); // see cubemap.vert
	glBindVertexArray(0);
}

void Cubemap::bind_to_unit(uint32_t unit)
{
	glBindTextureUnit(unit, *m_cubemap);
}

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

void Cubemap::load_equirectangular(std::string_view path)
{
	rc::texture_handle flat_texture;
	int flat_width, flat_height, chan;
	auto data = stbi_loadf(path.data(), &flat_width, &flat_height, &chan, 3);
	if(data && chan == 3) {
			glCreateTextures(GL_TEXTURE_2D, 1, flat_texture.get());
			glTextureStorage2D(*flat_texture, 1, GL_RGB32F, flat_width, flat_height);
			glTextureSubImage2D(*flat_texture, 0, 0, 0, flat_width, flat_height, GL_RGB, GL_FLOAT, data);
			stbi_image_free(data);
	} else {
		fmt::print(stderr, "[cubemap] could not open equirectangular map file '{}'\n", path);
		stbi_image_free(data);
		return;
	}

	unsigned cube_size = (flat_height * 3) / 4;

	rc::texture_handle cubemap_to;
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, cubemap_to.get());
	glTextureStorage3D(*cubemap_to, 1, GL_RGBA16F, cube_size, cube_size, 6);
	set_tex_params(*cubemap_to);

	glUseProgram(*cubemap_load_shader);
	glBindTextureUnit(0, *flat_texture);
	glBindImageTexture(0, *cubemap_to, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(cube_size/32, cube_size/32, 6);

	m_cubemap = std::move(cubemap_to);
}

Cubemap Cubemap::integrate_diffuse_irradiance()
{
	Cubemap res;
	if (!m_cubemap)
		return res;

	unsigned irradiance_size = 32;

	rc::texture_handle irradiance_cube_to;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, irradiance_cube_to.get());
	glTextureStorage2D(*irradiance_cube_to, 1, GL_RGBA16F, irradiance_size, irradiance_size);
	set_tex_params(*irradiance_cube_to);

	glUseProgram(*compute_diffuse_irradiance_shader);
	glBindTextureUnit(0, *m_cubemap);
	glBindImageTexture(0, *irradiance_cube_to, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(irradiance_size/16, irradiance_size/16, 6);

	res.m_cubemap = std::move(irradiance_cube_to);
	return res;
}

Cubemap Cubemap::convolve_specular()
{
	Cubemap res;
	if (!m_cubemap)
		return res;

	rc::texture_handle cubemap_with_mips_to;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, cubemap_with_mips_to.get());

	GLint src_size, dst_size;
	glGetTextureLevelParameteriv(*m_cubemap, 0, GL_TEXTURE_WIDTH, &src_size);
	dst_size = std::min(src_size / 4, 256);
	int dst_numlevels = rc::math::num_mipmap_levels(dst_size, dst_size);
	glTextureStorage2D(*cubemap_with_mips_to, dst_numlevels, GL_RGBA16F, dst_size, dst_size);
	set_tex_params(*cubemap_with_mips_to, true);


	{
		// downsample original cubemap
		rc::framebuffer_handle src_fbo, dst_fbo;
		glCreateFramebuffers(1, src_fbo.get());
		glCreateFramebuffers(1, dst_fbo.get());

		for (int i = 0; i < 6; ++i) {
			glNamedFramebufferTextureLayer(*src_fbo, GL_COLOR_ATTACHMENT0, *m_cubemap, 0, i);
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
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, specular_cube_to.get());
	glTextureStorage2D(*specular_cube_to, dst_numlevels, GL_RGBA16F, dst_size, dst_size);

	// Copy 0th mipmap level into destination environment map.
	glCopyImageSubData(*cubemap_with_mips_to, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
	                   *specular_cube_to,     GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
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
