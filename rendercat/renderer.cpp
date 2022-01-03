#include <rendercat/common.hpp>
#include <rendercat/renderer.hpp>
#include <rendercat/scene.hpp>
#include <rendercat/uniform.hpp>
#include <rendercat/util/gl_screenshot.hpp>
#include <rendercat/util/gl_debug.hpp>
#include <rendercat/util/turbo_colormap.hpp>
#include <fmt/core.h>
#include <string>
#include <stdexcept>
#include <imgui.h>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding-aux/Meta.h>

#include <debug_draw.hpp>
#include <tracy/Tracy.hpp>

#include <zcm/ivec2.hpp>
#include <zcm/hash.hpp>
#include <zcm/mat3.hpp>
#include <zcm/exponential.hpp>
#include <zcm/matrix_transform.hpp>
#include <zcm/type_ptr.hpp>

using namespace gl45core;
#include <TracyOpenGL.hpp>

using namespace rc;

static GLint max_elements_vertices;
static GLint max_elements_indices;
static GLint max_color_samples;
static GLint max_depth_samples;
static GLint max_framebuffer_samples;
static GLint max_framebuffer_width;
static GLint max_framebuffer_height;
static GLint max_uniform_locations;
static GLenum frag_derivative_quality_hint;

Renderer::Renderer(Scene& s, ShaderSet& shader_set) : m_shader_set(shader_set), m_scene(&s)
{
	m_shader = m_shader_set.load_program({"generic.vert", "generic.frag"});
	m_hdr_shader = m_shader_set.load_program({"hdr.comp"});
	m_bloom_downscale_shader = m_shader_set.load_program({"downscale_bloom_luma.comp"});

	glEnable(GL_FRAMEBUFFER_SRGB);

	glGetIntegerv(GL_MAX_SAMPLES, &MSAASampleCountMax);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &max_elements_vertices);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,  &max_elements_indices);
	glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_samples);
	glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_samples);
	glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES,   &max_framebuffer_samples);
	glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH,     &max_framebuffer_width);
	glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT,    &max_framebuffer_height);
	glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS,     &max_uniform_locations);
	glGetIntegerv(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, &frag_derivative_quality_hint);
#ifndef NDEBUG
	fmt::print(stderr,
	           "[renderer] limits:\n"
	           "    MSAA samples:        {} (color: {} depth: {} FB: {})\n"
	           "    Framebuffer size:    {} x {}\n"
	           "    Vertices:            {}\n"
	           "    Indices:             {}\n"
	           "    Uniform Locations:   {}\n"
	           "    Fragment Derivative: {}\n",
	           MSAASampleCountMax,
	           max_depth_samples,
	           max_color_samples,
	           max_framebuffer_samples,
	           max_framebuffer_width,
	           max_framebuffer_height,
	           max_elements_vertices,
	           max_elements_indices,
	           max_uniform_locations,
	           glbinding::aux::Meta::getString(frag_derivative_quality_hint));
#endif

	m_per_frame.set_label("per-frame generic uniforms");
	m_light_per_frame.set_label("per-frame light uniforms");
	m_tonemap_uniform.set_label("per-frame tonemap uniforms");

	dd::initialize(&debug_draw_ctx);
	init_shadow_resources();
	init_brdf();
	init_colormap();
	init_fsr();
}

void Renderer::init_shadow_resources()
{
	ZoneScoped;
	TracyGpuZone("init_shadow_resources");
	RC_DEBUG_GROUP("init_shadow_resources");
	m_shadow_shader = m_shader_set.load_program({"shadow_mapping.vert", "shadow_mapping.frag"});
	m_shadow_point_shader = m_shader_set.load_program({"shadow_mapping.vert", "shadow_mapping.frag"},
	                                                   {{"POINT_LIGHT"}});

	// create texture for directional light shadowmap
	glCreateTextures(GL_TEXTURE_2D, 1, m_shadowmap_depth_to.get());
	rcObjectLabel(m_shadowmap_depth_to, "directional shadow depth");
	glTextureStorage2D(*m_shadowmap_depth_to, 1, GL_DEPTH_COMPONENT32F, ShadowMapWidth, ShadowMapHeight);

	auto set_shadow_sampling_params = [](auto texture){
		float borderColor[] = {0.0f, 0.0f, 0.0f, 1.0f };
		glTextureParameterfv(texture, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTextureParameteri(texture, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTextureParameteri(texture, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	};
	set_shadow_sampling_params(*m_shadowmap_depth_to);

	glCreateFramebuffers(1, m_shadowmap_fbo.get());
	rcObjectLabel(m_shadowmap_fbo, "directional shadow FBO");
	glNamedFramebufferTexture(*m_shadowmap_fbo, GL_DEPTH_ATTACHMENT, *m_shadowmap_depth_to, 0);

	if (glCheckNamedFramebufferStatus(*m_shadowmap_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not init shadow framebuffer!");
		std::fflush(stderr);
	}

	// create texture array for spot light shadowmaps
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, m_spot_shadow_depth_to.get());
	rcObjectLabel(m_spot_shadow_depth_to, "spot shadow depth");
	glTextureStorage3D(*m_spot_shadow_depth_to, 1, GL_DEPTH_COMPONENT16, PointShadowWidth, PointShadowHeight, MaxLights);
	set_shadow_sampling_params(*m_spot_shadow_depth_to);

	glCreateFramebuffers(1, m_spot_shadow_fbo.get());
	rcObjectLabel(m_spot_shadow_fbo, "spot shadow FBO");
	glNamedFramebufferTexture(*m_spot_shadow_fbo, GL_DEPTH_ATTACHMENT, *m_spot_shadow_depth_to, 0);
	if (glCheckNamedFramebufferStatus(*m_spot_shadow_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not init spot shadow framebuffer!");
		std::fflush(stderr);
	}

	// create framebuffers for clearing individual layers in spot shadow texture array
	for (int i = 0; i < MaxLights; ++i) {
		glCreateFramebuffers(1, m_spot_layer_fbos[i].get());
		rcObjectLabel(m_spot_layer_fbos[i], fmt::format("spot clearing FBO #{}", i));
		glNamedFramebufferTextureLayer(*m_spot_layer_fbos[i], GL_DEPTH_ATTACHMENT, *m_spot_shadow_depth_to, 0, i);
		if (glCheckNamedFramebufferStatus(*m_spot_layer_fbos[i], GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			fmt::print(stderr, "[renderer] could not init spot shadow clearing framebuffer!");
			std::fflush(stderr);
		}
	}

	// create texture array for point light shadowmaps
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, m_point_shadow_depth_to.get());
	rcObjectLabel(m_point_shadow_depth_to, "point shadow depth");
	glTextureStorage3D(*m_point_shadow_depth_to, 1, GL_DEPTH_COMPONENT16, PointShadowWidth, PointShadowHeight, MaxLights * 6);
	set_shadow_sampling_params(*m_point_shadow_depth_to);
	glTextureParameteri(*m_point_shadow_depth_to, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);

	glCreateFramebuffers(1, m_point_shadow_fbo.get());
	rcObjectLabel(m_point_shadow_fbo, "point shadow FBO");
	glNamedFramebufferTexture(*m_point_shadow_fbo, GL_DEPTH_ATTACHMENT, *m_point_shadow_depth_to, 0);

	if (glCheckNamedFramebufferStatus(*m_point_shadow_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not init point shadow framebuffer!");
		std::fflush(stderr);
	}

	// create framebuffers for clearing individual layers in point shadow texture array
	for (int i = 0; i < MaxLights * 6; ++i) {
		glCreateFramebuffers(1, m_point_layer_fbos[i].get());
		rcObjectLabel(m_point_layer_fbos[i], fmt::format("point clearing FBO #{}", i));
		glNamedFramebufferTextureLayer(*m_point_layer_fbos[i], GL_DEPTH_ATTACHMENT, *m_point_shadow_depth_to, 0, i);
		if (glCheckNamedFramebufferStatus(*m_point_layer_fbos[i], GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			fmt::print(stderr, "[renderer] could not init point shadow clearing framebuffer!");
			std::fflush(stderr);
		}
	}
}

void Renderer::init_brdf()
{
	ZoneScoped;
	TracyGpuZone("init_brdf");
	RC_DEBUG_GROUP("init_brdf");

	m_brdf_shader = m_shader_set.load_program({"brdf_lut.comp"});
	if (!m_brdf_shader) {
		fmt::print(stderr, "[renderer] could not init BRDF LUT compute shader!\n");
		std::fflush(stderr);
		return;
	}

	int size = 256;

	glCreateTextures(GL_TEXTURE_2D, 1, m_brdf_lut_to.get());
	rcObjectLabel(m_brdf_lut_to, "BRDF LUT");
	glTextureStorage2D(*m_brdf_lut_to, 1, GL_RG16F, size, size);

	glBindImageTexture(0, *m_brdf_lut_to, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);
	glUseProgram(*m_brdf_shader);
	glDispatchCompute(size/8, size/8, 1);
	m_shader_set.deleteProgram(&m_brdf_shader);
	glTextureParameteri(*m_brdf_lut_to, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*m_brdf_lut_to, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*m_brdf_lut_to, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(*m_brdf_lut_to, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void Renderer::init_colormap()
{
	ZoneScoped;
	TracyGpuZone("init_colormap");
	RC_DEBUG_GROUP("init_colormap");
	glCreateTextures(GL_TEXTURE_1D, 1, m_turbo_colormap_to.get());
	rcObjectLabel(m_turbo_colormap_to, "turbo colormap");
	glTextureStorage1D(*m_turbo_colormap_to, 1, GL_RGB32F, 256);
	glTextureSubImage1D(*m_turbo_colormap_to, 0, 0, 256, GL_RGB, GL_FLOAT, &rc::util::turbo_srgb_floats);
}

void Renderer::init_fsr()
{
	ZoneScoped;
	m_fsr_easu_shader = m_shader_set.load_program({"fsr_pass1_easu.comp"});
	if (unlikely(!m_fsr_easu_shader)) {
		fmt::print(stderr, "[renderer] could not init FSR EASU compute shader!\n");
		std::fflush(stderr);
		return;
	}
	m_fsr_rcas_shader = m_shader_set.load_program({"fsr_pass2_rcas.comp"});
	if (unlikely(!m_fsr_rcas_shader)) {
		fmt::print(stderr, "[renderer] could not init FSR RCAS compute shader!\n");
		std::fflush(stderr);
		return;
	}
}

void Renderer::resize(uint32_t width, uint32_t height, float device_pixel_ratio)
{
	if(width == m_window_width && height == m_window_height)
		return;

	if(width < 2 || height < 2)
		return;

	ZoneScoped;
	TracyGpuZone("resize_backbuffer");
	RC_DEBUG_GROUP("resize_backbuffer");

	m_device_pixel_ratio = device_pixel_ratio;

	if(unlikely(width > (GLuint)max_framebuffer_width || height > (GLuint)max_framebuffer_height))
		throw std::runtime_error("framebuffer resize failed: specified size too big");

	// NOTE: for some reason AMD driver accepts 0 samples, but it's wrong
	int samples = zcm::clamp((int)zcm::exp2(desired_msaa_level), 1, MSAASampleCountMax);
	uint32_t backbuffer_width = width;
	uint32_t backbuffer_height = height;

	// reinitialize multisampled color texture object
	rc::texture_handle color_to;
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, color_to.get());
	rcObjectLabel(color_to, fmt::format("backbuffer MSAA color ({}x{}, {} samples)",
	                                    backbuffer_width, backbuffer_height, samples));
	glTextureStorage2DMultisample(*color_to,
	                              samples,
	                              GL_RGBA16F,
	                              backbuffer_width,
	                              backbuffer_height,
	                              GL_FALSE);

	// reinitialize multisampled depth texture object
	rc::texture_handle depth_to;
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, depth_to.get());
	rcObjectLabel(depth_to, fmt::format("backbuffer MSAA depth ({}x{}, {} samples)",
	                                    backbuffer_width, backbuffer_height, samples));
	glTextureStorage2DMultisample(*depth_to,
	                              samples,
	                              GL_DEPTH_COMPONENT32F, // TODO: research if GL_DEPTH24_STENCIL8 is viable here
	                              backbuffer_width,
	                              backbuffer_height,
	                              GL_FALSE);

	if(!color_to || !depth_to) {
		fmt::print(stderr, "[renderer] backbuffer {} texture creation failed\n", color_to ? "depth" : "color");
		std::fflush(stderr);
		return;
	}

	// reinitialize framebuffer
	rc::framebuffer_handle fbo;
	glCreateFramebuffers(1, fbo.get());
	rcObjectLabel(fbo,"backbuffer MSAA FBO");
	// attach texture objects
	glNamedFramebufferTexture(*fbo, GL_COLOR_ATTACHMENT0, *color_to, 0);
	glNamedFramebufferTexture(*fbo, GL_DEPTH_ATTACHMENT,  *depth_to, 0);

	if (glCheckNamedFramebufferStatus(*fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not resize backbuffer!");
		std::fflush(stderr);
		return;
	}

	rc::texture_handle resolve_to;
	glCreateTextures(GL_TEXTURE_2D, 1, resolve_to.get());
	rcObjectLabel(resolve_to, fmt::format("backbuffer color resolve ({}x{})",
	                                      backbuffer_width, backbuffer_height));

	glTextureStorage2D(*resolve_to, 1, GL_RGBA16F, backbuffer_width, backbuffer_height);

	rc::framebuffer_handle resolve_fbo;
	glCreateFramebuffers(1, resolve_fbo.get());
	rcObjectLabel(resolve_fbo, "backbuffer resolve FBO");
	glNamedFramebufferTexture(*resolve_fbo, GL_COLOR_ATTACHMENT0, *resolve_to, 0);

	if (glCheckNamedFramebufferStatus(*resolve_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not resize resolve backbuffer!");
		std::fflush(stderr);
		return;
	}

	int downscaled_width = backbuffer_width / 2;
	int downscaled_height = backbuffer_height / 2;

	rc::texture_handle resolve_downscale_to;
	glCreateTextures(GL_TEXTURE_2D, 1, resolve_downscale_to.get());
	rcObjectLabel(resolve_downscale_to, fmt::format("backbuffer color resolve downscale ({}x{})",
	                                      downscaled_width, downscaled_height));

	glTextureStorage2D(*resolve_downscale_to,
	                   std::min(NumMipsBloomDownscale, rc::math::num_mipmap_levels(downscaled_width, downscaled_height)),
	                   GL_RGBA16F,
	                   downscaled_width,
	                   downscaled_height);

	glTextureParameteri(*resolve_downscale_to, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTextureParameteri(*resolve_downscale_to, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTextureParameteri(*resolve_downscale_to, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(*resolve_downscale_to, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	// ------------

	rc::texture_handle ldr_tonemapped_to;
	glCreateTextures(GL_TEXTURE_2D, 1, ldr_tonemapped_to.get());
	rcObjectLabel(ldr_tonemapped_to, fmt::format("LDR tonemapped ({}x{})", backbuffer_width, backbuffer_height));
	glTextureStorage2D(*ldr_tonemapped_to, 1, GL_RGB10_A2, backbuffer_width, backbuffer_height);


	rc::framebuffer_handle ldr_tonemap_fbo;
	glCreateFramebuffers(1, ldr_tonemap_fbo.get());
	rcObjectLabel(ldr_tonemap_fbo, "LDR tonemap FBO");
	glNamedFramebufferTexture(*ldr_tonemap_fbo, GL_COLOR_ATTACHMENT0, *ldr_tonemapped_to, 0);
	if (glCheckNamedFramebufferStatus(*ldr_tonemap_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not resize LDR tonemap backbuffer!");
		std::fflush(stderr);
		return;
	}

	rc::texture_handle fsr_pass1_easu_to;
	glCreateTextures(GL_TEXTURE_2D, 1, fsr_pass1_easu_to.get());
	rcObjectLabel(fsr_pass1_easu_to, fmt::format("FSR pass1 easu ({}x{})", width, height));
	glTextureStorage2D(*fsr_pass1_easu_to, 1, GL_RGB10_A2UI, width, height);

	rc::texture_handle fsr_pass2_rcas_to;
	glCreateTextures(GL_TEXTURE_2D, 1, fsr_pass2_rcas_to.get());
	rcObjectLabel(fsr_pass2_rcas_to, fmt::format("FSR pass2 rcas ({}x{})", width, height));
	glTextureStorage2D(*fsr_pass2_rcas_to, 1, GL_RGBA8, width, height);

	rc::framebuffer_handle fsr_pass2_rcas_fbo;
	glCreateFramebuffers(1, fsr_pass2_rcas_fbo.get());
	rcObjectLabel(fsr_pass2_rcas_fbo, "FSR pass1 easu FBO");
	glNamedFramebufferTexture(*fsr_pass2_rcas_fbo, GL_COLOR_ATTACHMENT0, *fsr_pass2_rcas_to, 0);
	if (glCheckNamedFramebufferStatus(*fsr_pass2_rcas_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not resize FSR pass1 easu backbuffer!");
		std::fflush(stderr);
		return;
	}

	assert(glGetError() == GL_NO_ERROR);

	m_backbuffer_fbo = std::move(fbo);
	m_backbuffer_color_to = std::move(color_to);
	m_backbuffer_depth_to = std::move(depth_to);
	m_backbuffer_resolve_fbo = std::move(resolve_fbo);
	m_backbuffer_resolve_color_to = std::move(resolve_to);
	m_bloom_color_to = std::move(resolve_downscale_to);
	m_ldr_tonemapped_to = std::move(ldr_tonemapped_to);
	m_ldr_tonemap_fbo = std::move(ldr_tonemap_fbo);
	m_fsr_pass1_easu_to = std::move(fsr_pass1_easu_to);
	m_fsr_pass2_rcas_to = std::move(fsr_pass2_rcas_to);
	m_fsr_pass2_rcas_fbo = std::move(fsr_pass2_rcas_fbo);

	msaa_level = desired_msaa_level;
	MSAASampleCount = samples;
	m_window_width = width;
	m_window_height = height;

	m_scene->main_camera.state.aspect = (float(m_window_width) / (float)m_window_height);
	debug_draw_ctx.window_size = zcm::vec2{(float)backbuffer_width, (float)backbuffer_height};
	fmt::print(stderr, "[renderer] framebuffer resized to {}x{}\n", backbuffer_width, backbuffer_height);
	std::fflush(stderr);
}

void Renderer::clear_screen()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_window_width, m_window_height);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
#if 0
static void drawFullscreenTriangle()
{
	static GLuint vao;
	if (unlikely(vao == 0)) {
		glCreateVertexArrays(1, &vao);
		rcObjectLabel(GL_VERTEX_ARRAY, vao, "single triangle VAO");
	}
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}
#endif


static void check_and_block_sync(rc::sync_handle sync, const char* message=nullptr) {
	ZoneScoped;
	if (likely(sync != nullptr)) {
		auto result = glClientWaitSync(*sync, gl::GL_NONE_BIT, 0);
		if (result != GL_ALREADY_SIGNALED) {
			ZoneScopedNC("block on sync", 0xff0000);
			if (message) {
				TracyMessageLC(message, 0xff0000);
			}
			glClientWaitSync(*sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000000);
		}
	}
}

void Renderer::set_uniforms()
{
	ZoneScoped;

	zcm::mat4 proj_view;
	{
		ZoneScopedN("calc camera matrices");
		m_scene->main_camera.update();

		auto view = make_view(m_scene->main_camera.state);
		auto projection = make_projection(m_scene->main_camera.state);
		proj_view = projection * view;
		debug_draw_ctx.mvpMatrix = proj_view;
	}

	check_and_block_sync(m_per_frame.next(), "Per-frame uniform sync triggered, blocking!");

	auto per_frame = m_per_frame.data();
	assert(per_frame);
	m_per_frame.bind(1);

	per_frame->proj_view = proj_view;
	per_frame->light_proj_view = m_shadow_matrix;
	per_frame->znear           = m_scene->main_camera.state.znear;
	per_frame->camera_forward  = -m_scene->main_camera.state.get_backward();
	per_frame->viewPos         = m_scene->main_camera.state.position;

	per_frame->dir_light[0].color_intensity = m_scene->directional_light.color_intensity;
	per_frame->dir_light[0].direction       = m_scene->directional_light.direction;

	per_frame->dir_fog.inscattering_opacity     = m_scene->fog.inscattering_environment_opacity;
	per_frame->dir_fog.dir_inscattering_color = m_scene->fog.dir_inscattering_color;
	per_frame->dir_fog.direction_exponent     = zcm::vec4{-m_scene->directional_light.direction, m_scene->fog.dir_exponent};
	per_frame->dir_fog.inscattering_density   = m_scene->fog.inscattering_density;
	per_frame->dir_fog.extinction_density     = m_scene->fog.extinction_density;
	per_frame->dir_fog.enabled                = (m_scene->fog.state & ExponentialDirectionalFog::Enabled);

	for(unsigned i = 0; i < m_scene->point_lights.size() && i < MaxLights;++i) {
		const auto& light = m_scene->point_lights[i];
		per_frame->point_lights[i].position_radius = zcm::vec4{light.data.position, light.data.radius};
		per_frame->point_lights[i].color_intensity = zcm::vec4{light.data.color, light.data.intensity};
	}

	for(unsigned i = 0; i < m_scene->spot_lights.size() && i < MaxLights;++i) {
		const auto& light = m_scene->spot_lights[i];
		per_frame->spot_lights[i].position_radius = zcm::vec4{light.data.position, light.data.radius};
		per_frame->spot_lights[i].color_intensity = zcm::vec4{light.data.color, light.data.intensity};
		per_frame->spot_lights[i].direction_angle_scale = zcm::vec4{light.direction_vec(), light.data.angle_scale};
		per_frame->spot_lights[i].angle_offset = zcm::vec4{light.angle_offset()};
	}

	per_frame->num_msaa_samples = MSAASampleCount;

	const uint32_t SHADOWS_DIRECTIONAL          = 1 << 1;
	const uint32_t SHADOWS_POINT                = 1 << 2;
	const uint32_t SHADOWS_SPOT                 = 1 << 3;

	uint32_t flags = 0;
	if (do_shadow_mapping) {
		bool do_directional = enable_directional_shadows && m_scene->directional_light.color_intensity.w > 0.0f;
		flags |= do_directional ? SHADOWS_DIRECTIONAL : 0;
		flags |= enable_point_shadows       ? SHADOWS_POINT : 0;
		flags |= enable_spot_shadows        ? SHADOWS_SPOT : 0;
	}
	per_frame->flags = flags;

	m_per_frame.flush();

	// ------

	Texture::bind_to_unit(m_scene->cubemap_diffuse_irradiance, 34);
	Texture::bind_to_unit(m_scene->cubemap_specular_environment, 33);
	glBindTextureUnit(30, *m_turbo_colormap_to);
	glBindTextureUnit(31, *m_brdf_lut_to);
}


template<size_t MaxLights>
static int process_point_lights(const std::vector<PointLight>& point_lights,
                                 const Frustum& frustum,
                                 const bbox3& submesh_bbox,
                                 uint32_t shader)
{
	int point_light_count = 0;
	for(unsigned i = 0; i < point_lights.size() && i < MaxLights; ++i) {
		const auto& light = point_lights[i];
		if(!(light.state & PointLight::Enabled))
			continue;
		if(frustum.sphere_culled(light.position(), light.radius()))
			continue;

		// TODO: implement per-light AABB cutoff to prevent light leaking
		if(bbox3::intersects_sphere(submesh_bbox, light.position(), light.radius()) != Intersection::Outside) {
			auto dist = zcm::length(light.position() - submesh_bbox.closest_point(light.position()));
			if(light.distance_attenuation(dist) > 0.0f) {
				unif::i1(shader, 10 + point_light_count++, i);
			}
		}
	}
	unif::i1(shader, "num_point_lights", point_light_count);
	return point_light_count;
}

template<size_t MaxLights>
static int process_spot_lights(const std::vector<SpotLight>& spot_lights,
                                const Frustum& frustum,
                                const bbox3& submesh_bbox,
                                uint32_t shader)
{
	int spot_light_count = 0;
	for(unsigned i = 0; i < spot_lights.size() && i < MaxLights; ++i) {
		const auto& light = spot_lights[i];
		if(!(light.state & SpotLight::Enabled))
			continue;

		if(frustum.sphere_culled(light.position(), light.radius()))
			continue;

		if(bbox3::intersects_cone(submesh_bbox,
		                          light.position(),
		                          -light.direction_vec(),
		                          light.angle_outer(),
		                          light.radius()) != Intersection::Outside) {
			auto dist = zcm::length(light.position() - submesh_bbox.closest_point(light.position()));
			if(light.distance_attenuation(dist) > 0.0f) {
				unif::i1(shader, 10 + MaxLights + spot_light_count++, i);
			}
		}
	}
	unif::i1(shader, "num_spot_lights", spot_light_count);
	return spot_light_count;
}

template<bool instanced=false>
static void submit_draw_call(const model::Mesh& submesh, int num_instances=1)
{
	glBindVertexArray(*submesh.vao);

	if (likely(submesh.index_type)) {

		assert(submesh.index_type == GL_UNSIGNED_INT
		       || submesh.index_type == GL_UNSIGNED_SHORT
		       || submesh.index_type == GL_UNSIGNED_BYTE);

		if constexpr(!instanced) {

			if (likely(submesh.index_max > submesh.index_min)) {

				glDrawRangeElements((GLenum)submesh.draw_mode,
				                    submesh.index_min,
				                    submesh.index_max,
				                    submesh.numverts,
				                    GLenum(submesh.index_type),
				                    nullptr);

			} else {

				glDrawElements((GLenum)submesh.draw_mode,
				               submesh.numverts,
				               GLenum(submesh.index_type),
				               nullptr);
			}

		} else { // instanced case
			glDrawElementsInstanced((GLenum)submesh.draw_mode,
			                        submesh.numverts,
			                        GLenum(submesh.index_type),
			                        nullptr,
			                        num_instances);

		}
	} else {
		if constexpr(!instanced) {
			glDrawArrays((GLenum)submesh.draw_mode, 0, submesh.numverts);
		} else {
			glDrawArraysInstanced((GLenum)submesh.draw_mode, 0, submesh.numverts, num_instances);
		}
	}
}

static void render_generic(const model::Mesh& submesh,
                           const Material& material,
                           uint32_t shader)
{
	if(material.double_sided()) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
	}

	material.bind(shader);
	unif::b1(shader, "has_tangents", submesh.has_tangents);
	submit_draw_call(submesh);
}


void Renderer::draw_directional_shadow()
{
	ZoneScoped;
	TracyGpuZone("draw_shadow");
	RC_DEBUG_GROUP("directional shadow");
	glClearDepthf(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // TODO: use front face culling

	glBindFramebuffer(GL_FRAMEBUFFER, *m_shadowmap_fbo);
	glViewport(0,0, ShadowMapWidth, ShadowMapHeight);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(*m_shadow_shader);

	// TODO: determine frustum size dynamically
	static float near_plane = -25.0f, far_plane = 25.0f;
	static zcm::vec4 params = zcm::vec4(-15.0f, 15.0f, -15.0f, 15.0f);
	auto pos = m_scene->directional_light.direction;

	zcm::mat4 lightProjection = zcm::orthoRH_ZO(params[0], params[1], params[2],params[3], near_plane, far_plane);
	auto lightView = zcm::lookAtRH(pos,
	                             zcm::vec3(0.0f, 0.0f, 0.0f),
	                             zcm::vec3(0.0f, 1.0f, 0.0f));

	auto proj_view = lightProjection * lightView;

	unif::m4(*m_shadow_shader, 4, proj_view);
	m_shadow_matrix = proj_view;
	unif::b1(*m_shadow_shader, 0, false); // alpha-masked

	{
		RC_DEBUG_GROUP("opaque meshes");
		for (const auto& idx : m_opaque_meshes) {
			const MeshTransform& transform = m_transform_cache[idx.transform_idx];

			const auto& shaded_mesh = m_scene->shaded_meshes[idx.submesh_idx];
			const model::Mesh& submesh = m_scene->submeshes[shaded_mesh.mesh];

			unif::m4(*m_shadow_shader, 1, transform.mat);
			submit_draw_call(submesh);
		}
	}
	{
		RC_DEBUG_GROUP("masked meshes");
		unif::b1(*m_shadow_shader, 0, true); // alpha-masked

		for (const auto& idx : m_masked_meshes) {
			const MeshTransform& transform = m_transform_cache[idx.transform_idx];

			const auto& shaded_mesh = m_scene->shaded_meshes[idx.submesh_idx];
			const model::Mesh& submesh = m_scene->submeshes[shaded_mesh.mesh];
			const auto& material = m_scene->materials[shaded_mesh.material];

			unif::m4(*m_shadow_shader, 1, transform.mat);
			material.bind(*m_shadow_shader);
			submit_draw_call(submesh);
		}
	}

	glUseProgram(0);
	glBindVertexArray(0);
}

Renderer::LightPerframeData * Renderer::begin_draw_light_shadows()
{
	glClearDepthf(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // TODO: use front face culling

	check_and_block_sync(m_light_per_frame.next(), "Light per-frame uniform sync triggered, blocking!");
	auto per_frame = m_light_per_frame.data();
	assert(per_frame);
	m_light_per_frame.bind(2);
	return per_frame;
}


static bool light_culled_by_bbox(const PointLight& light, const bbox3& bbox) {
	return bbox3::intersects_sphere(bbox,
	                              light.position(),
	                              light.radius()) == Intersection::Outside;
}


template<typename LightT, typename Cont>
static size_t light_hash(const LightT& light, const Cont& transform_cache) {
	size_t transform_hash = zcm::hash(light.position());
	transform_hash ^= zcm::hash(light.radius());

	if constexpr(std::is_same_v<LightT, SpotLight>) {
		transform_hash ^= zcm::hash(light.orientation());
	}

	for (const auto& transform : transform_cache) {
		if (light_culled_by_bbox(light, transform.transformed_bbox))
			continue;
		transform_hash ^= zcm::hash(transform.transformed_bbox.min());
		transform_hash ^= zcm::hash(transform.transformed_bbox.max());
	}
	return transform_hash;
}


void Renderer::draw_point_shadow(Renderer::LightPerframeData *per_frame)
{
	ZoneScoped;
	TracyGpuZone("draw_point_shadow");
	RC_DEBUG_GROUP("point shadows");

	glViewport(0,0, PointShadowWidth, PointShadowHeight);
	glUseProgram(*m_shadow_point_shader);
	if (!enable_shadow_caching) {
		glBindFramebuffer(GL_FRAMEBUFFER, *m_point_shadow_fbo);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	const float near = 0.1f;
	int point_shadowmap_count = 0;
	int updated_count = 0;

	for(size_t scene_index = 0; scene_index < m_scene->point_lights.size() && scene_index < MaxLights; ++scene_index) {
		const auto& light = m_scene->point_lights[scene_index];

		RC_DEBUG_GROUP(fmt::format("point light {} (#{})", scene_index, point_shadowmap_count));

		if(!(light.state & SpotLight::Enabled))
			continue;

		if(m_scene->main_camera.frustum.sphere_culled(light.position(), light.radius()))
			continue;

		if(light.state & PointLight::ShowWireframe) {
			dd::sphere(light.position(), light.color(), 0.01f * light.radius(), 0, false);
			dd::sphere(light.position(), light.color(), light.radius());
		}

		if (enable_shadow_caching) {
			size_t transform_hash = light_hash(light, m_transform_cache);
			if (transform_hash == m_point_light_hashes[scene_index]) {
				++point_shadowmap_count;
				continue;
			}
			m_point_light_hashes[scene_index] = transform_hash;

			{
				ZoneScopedN("Clear layer");
				TracyGpuZone("Clear layer")
				for (int i = 0; i < 6; ++i) {
					glBindFramebuffer(GL_FRAMEBUFFER, *m_point_layer_fbos[scene_index*6 + i]);
					glClear(GL_DEPTH_BUFFER_BIT);
				}
				glBindFramebuffer(GL_FRAMEBUFFER, *m_point_shadow_fbo);
			}
		}
		++updated_count;


		const zcm::mat4 proj = zcm::perspectiveRH_ZO(zcm::radians(90.0f), 1, near, light.radius());
		const std::array<zcm::mat4, 6> shadowTransforms {
			proj * zcm::lookAtRH(light.position(), light.position() + zcm::vec3( 1.0, 0.0, 0.0), zcm::vec3(0.0,-1.0, 0.0)),
			proj * zcm::lookAtRH(light.position(), light.position() + zcm::vec3(-1.0, 0.0, 0.0), zcm::vec3(0.0,-1.0, 0.0)),
			proj * zcm::lookAtRH(light.position(), light.position() + zcm::vec3( 0.0, 1.0, 0.0), zcm::vec3(0.0, 0.0, 1.0)),
			proj * zcm::lookAtRH(light.position(), light.position() + zcm::vec3( 0.0,-1.0, 0.0), zcm::vec3(0.0, 0.0,-1.0)),
			proj * zcm::lookAtRH(light.position(), light.position() + zcm::vec3( 0.0, 0.0, 1.0), zcm::vec3(0.0,-1.0, 0.0)),
			proj * zcm::lookAtRH(light.position(), light.position() + zcm::vec3( 0.0, 0.0,-1.0), zcm::vec3(0.0,-1.0, 0.0))
		};

		const std::array<ShadowFrustum, 6> shadowFrusta {
			ShadowFrustum{light.position(), zcm::vec3( 1.0, 0.0, 0.0), zcm::vec3(0.0,-1.0, 0.0), near, light.radius()},
			ShadowFrustum{light.position(), zcm::vec3(-1.0, 0.0, 0.0), zcm::vec3(0.0,-1.0, 0.0), near, light.radius()},
			ShadowFrustum{light.position(), zcm::vec3( 0.0, 1.0, 0.0), zcm::vec3(0.0, 0.0, 1.0), near, light.radius()},
			ShadowFrustum{light.position(), zcm::vec3( 0.0,-1.0, 0.0), zcm::vec3(0.0, 0.0,-1.0), near, light.radius()},
			ShadowFrustum{light.position(), zcm::vec3( 0.0, 0.0, 1.0), zcm::vec3(0.0,-1.0, 0.0), near, light.radius()},
			ShadowFrustum{light.position(), zcm::vec3( 0.0, 0.0,-1.0), zcm::vec3(0.0,-1.0, 0.0), near, light.radius()}
		};

		for (int i = 0; i < 6; ++i) {
			unif::m4(*m_shadow_point_shader, 4 + i, shadowTransforms[i]);
		}

		unif::b1(*m_shadow_point_shader, 0, false); // alpha-masked
		unif::i1(*m_shadow_point_shader, 2, scene_index);

		auto face_culled = [&shadowFrusta](const auto& bbox, int index) {
			return shadowFrusta[index].bbox_culled(bbox);
		};

		auto process_mesh = [this, &face_culled](const auto& meshes, const auto& light, bool use_material=false){
			for (const auto& idx : meshes) {
				const MeshTransform& transform = m_transform_cache[idx.transform_idx];

				if (light_culled_by_bbox(light, transform.transformed_bbox))
					continue;

				int num_faces = 0;
				for (int i = 0; i < 6; ++i) {
					if (!face_culled(transform.transformed_bbox, i)) {
						unif::i1(*m_shadow_point_shader, 11+num_faces, i);
						++num_faces;
					}
				}

				if (num_faces == 0)
					continue;

				const auto& shaded_mesh = m_scene->shaded_meshes[idx.submesh_idx];
				const model::Mesh& submesh = m_scene->submeshes[shaded_mesh.mesh];
				if (use_material) {
					const auto& material = m_scene->materials[shaded_mesh.material];
					material.bind(*m_shadow_point_shader);
				}

				unif::m4(*m_shadow_point_shader, 1, transform.mat);
				submit_draw_call<true>(submesh, num_faces);
			}
		};



		{
			RC_DEBUG_GROUP("opaque meshes");
			process_mesh(m_opaque_meshes, light);

		}

		{
			RC_DEBUG_GROUP("masked meshes");
			unif::b1(*m_shadow_point_shader, 0, true); // alpha-masked
			process_mesh(m_masked_meshes, light, true);
		}

		++point_shadowmap_count;
	}
	per_frame->num_visible_point_lights = point_shadowmap_count;
	per_frame->point_near_plane = near;
	TracyPlot("Visible point lights", int64_t(point_shadowmap_count));
	TracyPlot("Updated point lights", int64_t(updated_count));
}

void Renderer::draw_spot_shadow(LightPerframeData *per_frame)
{
	ZoneScoped;
	TracyGpuZone("draw_spot_shadow");
	RC_DEBUG_GROUP("spot shadows");

	glViewport(0,0, PointShadowWidth, PointShadowHeight);
	glUseProgram(*m_shadow_shader);
	if (!enable_shadow_caching) {
		glBindFramebuffer(GL_FRAMEBUFFER, *m_spot_shadow_fbo);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	int spot_shadowmaps_count = 0;
	int updated_spot_count = 0;
	for(size_t scene_index = 0; scene_index < m_scene->spot_lights.size() && scene_index < MaxLights; ++scene_index) {
		const auto& light = m_scene->spot_lights[scene_index];
		RC_DEBUG_GROUP(fmt::format("point light {} (#{})", scene_index, spot_shadowmaps_count));

		if(m_scene->main_camera.frustum.sphere_culled(light.position(), light.radius()))
			continue;

		if(light.state & SpotLight::ShowWireframe) {
			dd::cone(light.position(),
				 zcm::normalize(-light.direction_vec()) * light.radius(),
				 light.color(),
				 light.angle_outer() * light.radius(), 0.0f);
		}

		auto light_camera_state = CameraState{zcm::conjugate(light.orientation()), light.position()};
		light_camera_state.zfar = light.radius();
		light_camera_state.znear = 0.1f;
		light_camera_state.fov = light.angle_outer() * 1.9f;
		light_camera_state.aspect = 1.0f;
		light_camera_state.normalize();

		const auto view_mat = make_view(light_camera_state);
		const auto proj_mat = make_projection_non_reversed_z(light_camera_state);

		const auto light_mat = proj_mat * view_mat;
		per_frame->spot_light_matrices[scene_index] = light_mat;

		if (enable_shadow_caching) {
			size_t transform_hash = light_hash(light, m_transform_cache);
			if (transform_hash == m_spot_light_hashes[scene_index]) {
				++spot_shadowmaps_count;
				continue;
			}
			m_spot_light_hashes[scene_index] = transform_hash;
			{
				ZoneScopedN("Clear layer");
				TracyGpuZone("Clear layer")
				glBindFramebuffer(GL_FRAMEBUFFER, *m_spot_layer_fbos[scene_index]);
				glClear(GL_DEPTH_BUFFER_BIT);
				glBindFramebuffer(GL_FRAMEBUFFER, *m_spot_shadow_fbo);
			}
		}
		++updated_spot_count;

		unif::b1(*m_shadow_shader, 0, false); // alpha-masked
		unif::m4(*m_shadow_shader, 4, light_mat);
		unif::i1(*m_shadow_shader, 2, scene_index);

		auto frustum = Frustum();
		frustum.update(light_camera_state);

		auto spot_culled = [](const auto& bbox, const auto& light)
		{
			return bbox3::intersects_cone(bbox,
			                              light.position(),
			                              -light.direction_vec(),
			                              light.angle_outer(),
			                              light.radius()) == Intersection::Outside;
		};

		auto process_meshes = [this, &spot_culled, &frustum](const auto& meshes, const auto& light, bool use_material=false)
		{
			for (const auto& idx : meshes) {
				const MeshTransform& transform = m_transform_cache[idx.transform_idx];

				if (spot_culled(transform.transformed_bbox, light))
					continue;

				if (frustum.bbox_culled(transform.transformed_bbox))
					continue;

				const auto& shaded_mesh = m_scene->shaded_meshes[idx.submesh_idx];
				const model::Mesh& submesh = m_scene->submeshes[shaded_mesh.mesh];

				if (use_material) {
					const auto& material = m_scene->materials[shaded_mesh.material];
					material.bind(*m_shadow_shader);
				}

				unif::m4(*m_shadow_shader, 1, transform.mat);
				submit_draw_call(submesh);
			}
		};

		{
			RC_DEBUG_GROUP("opaque meshes");
			process_meshes(m_opaque_meshes, light);
		}

		{
			RC_DEBUG_GROUP("masked meshes");
			unif::b1(*m_shadow_shader, 0, true); // alpha-masked
			process_meshes(m_masked_meshes, light, true);
		}

		++spot_shadowmaps_count;
	}

	per_frame->num_visible_spot_lights = spot_shadowmaps_count;
	TracyPlot("Visible spot lights", int64_t(spot_shadowmaps_count));
	TracyPlot("Updated spot lights", int64_t(updated_spot_count));
}

void Renderer::end_draw_light_shadows()
{
	m_light_per_frame.flush();
	glUseProgram(0);
	glBindVertexArray(0);
}

void Renderer::bloom_pass()
{
	RC_DEBUG_GROUP("bloom pass");
	ZoneScoped;
	TracyGpuZone("bloom pass");
	auto render_size = _render_size();
	glBlitNamedFramebuffer(*m_backbuffer_fbo,
	                       *m_backbuffer_resolve_fbo,
	                       0, 0, render_size.x, render_size.y,
	                       0, 0, render_size.x, render_size.y,
	                       GL_COLOR_BUFFER_BIT,
	                       GL_NEAREST);

	auto read_texture = *m_backbuffer_resolve_color_to;

	// source texture scale
	int src_width = int(m_window_width) / 2;
	int src_height = int(m_window_height) / 2;

	// region in source texture for dispatching
	int dispatch_width = render_size.x / 2;
	int dispatch_height = render_size.y / 2;

	const auto levels = std::min(NumMipsBloomDownscale, rc::math::num_mipmap_levels(dispatch_width, dispatch_height));
	const auto RegionDim = 16;
	std::array<zcm::ivec2, NumMipsBloomDownscale> dispatch_dims;
	std::array<zcm::ivec2, NumMipsBloomDownscale> source_dims;

	glUseProgram(*m_bloom_downscale_shader);

	unif::i2(*m_bloom_downscale_shader, 0, -1, 0); // lvl: 0 mode: down
	unif::v2(*m_bloom_downscale_shader, 1, {1.0f / src_width, 1.0f / src_height});
	unif::v2(*m_bloom_downscale_shader, 2, {bloom_threshold, bloom_strength});

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT);

	{
		RC_DEBUG_GROUP("downsample");
		for (int level = 0; level < levels; ++level) {
			dispatch_dims[level] = {dispatch_width, dispatch_height};
			source_dims[level] = {src_width, src_height};

			glBindTextureUnit(0, read_texture);
			glBindImageTexture(0, *m_bloom_color_to, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);


			int dispatch_x = (dispatch_width + (RegionDim - 1)) / RegionDim;
			int dispatch_y = (dispatch_height+ (RegionDim - 1)) / RegionDim;
			glDispatchCompute(dispatch_x, dispatch_y, 1);
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			read_texture = *m_bloom_color_to;

			dispatch_width /= 2;
			dispatch_height /= 2;
			src_width /= 2;
			src_height /= 2;

			unif::i2(*m_bloom_downscale_shader, 0, level, 0); // downscale
			unif::v2(*m_bloom_downscale_shader, 1, {1.0f / src_width, 1.0f / src_height});
		}
	}

	{
		RC_DEBUG_GROUP("upsample");
		for (int level = levels-1; level > 0; level--) {

			dispatch_width = dispatch_dims[level-1].x;
			dispatch_height = dispatch_dims[level-1].y;
			src_width = source_dims[level-1].x;
			src_height = source_dims[level-1].y;

			glBindTextureUnit(0, *m_bloom_color_to);
			glBindImageTexture(0, *m_bloom_color_to, level-1, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			unif::i2(*m_bloom_downscale_shader, 0, level, 1); // upscale
			unif::v2(*m_bloom_downscale_shader, 1, {1.0f / src_width, 1.0f / src_height});

			int dispatch_x = (dispatch_width + (RegionDim - 1)) / RegionDim;
			int dispatch_y = (dispatch_height+ (RegionDim - 1)) / RegionDim;
			glDispatchCompute(dispatch_x, dispatch_y, 1);
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}
}

void Renderer::tonemap_pass()
{
	TracyGpuNamedZone(rresolve, "resolve and tonemap", true);
	RC_DEBUG_GROUP("resolve and tonemap");

	glUseProgram(*m_hdr_shader);
	glBindTextureUnit(0, *m_backbuffer_color_to);
	glBindTextureUnit(1, *m_bloom_color_to);
	glBindImageTexture(0, *m_ldr_tonemapped_to, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGB10_A2UI);

	check_and_block_sync(m_tonemap_uniform.next(), "Per-frame uniform sync triggered, blocking!");

	auto uniforms = m_tonemap_uniform.data();
	assert(uniforms);
	m_tonemap_uniform.bind(0);

	uniforms->exposure = m_scene->main_camera.state.exposure;
	uniforms->sample_count = MSAASampleCount;
	uniforms->bloom_strength = do_bloom ? bloom_strength : 0.0f;
	uniforms->render_scale = _render_scale();
	m_tonemap_uniform.flush();

	const auto RegionDim = 16;
	auto render_size = _render_size();
	int dispatch_x = (render_size.x + (RegionDim - 1)) / RegionDim;
	int dispatch_y = (render_size.y + (RegionDim - 1)) / RegionDim;
	glDispatchCompute(dispatch_x,
	                  dispatch_y,
	                  1);
}

void Renderer::upscale_pass()
{
	const auto RegionDim = 16;
	int dispatch_x = (m_window_width + (RegionDim - 1)) / RegionDim;
	int dispatch_y = (m_window_height+ (RegionDim - 1)) / RegionDim;

	{
		TracyGpuNamedZone(fsr_pass1, "FSR pass1 EASU", true);
		RC_DEBUG_GROUP("FSR pass1 EASU");

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(*m_fsr_easu_shader);
		glBindTextureUnit(0, *m_ldr_tonemapped_to);
		glBindImageTexture(0, *m_fsr_pass1_easu_to, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGB10_A2UI);

		unif::v2(*m_fsr_easu_shader, 0, zcm::vec2(m_window_width, m_window_height)); // source size
		unif::v2(*m_fsr_easu_shader, 1, zcm::floor(zcm::vec2(m_window_width, m_window_height) * _render_scale())); // viewport size
		unif::v2(*m_fsr_easu_shader, 2, zcm::vec2(m_window_width, m_window_height));  // dest size


		glDispatchCompute(dispatch_x,
				  dispatch_y,
				  1);

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	{
		TracyGpuNamedZone(fsr_pass1, "FSR pass2 RCAS & filmgrain", true);
		RC_DEBUG_GROUP("FSR pass2 EASU & filmgrain");
		glUseProgram(*m_fsr_rcas_shader);
		glBindImageTexture(0, *m_fsr_pass1_easu_to, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGB10_A2UI);
		glBindImageTexture(1, *m_fsr_pass2_rcas_to, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

		unif::v2(*m_fsr_rcas_shader, 0, zcm::vec2(fsr_sharpening, fsr_filmgrain)); // sharpening, filmgain
		unif::i2(*m_fsr_rcas_shader, 1, m_frame_number, 0); // frame num for noise
		glDispatchCompute(dispatch_x,
				  dispatch_y,
				  1);

		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT);
	}
}

float Renderer::_render_scale() const noexcept {
	return zcm::clamp(desired_render_scale, 0.1f, 1.0f);
}

zcm::ivec2 Renderer::_render_size() const noexcept
{
	return zcm::ivec2{static_cast<int>(m_window_width * _render_scale()),
	                  static_cast<int>(m_window_height * _render_scale())};
}

void Renderer::draw()
{
	if(unlikely(!m_shader)) return;

	++m_frame_number;

	ZoneScoped;
	m_shader_set.check_updates();

	// clear all caches
	m_opaque_meshes.clear();
	m_masked_meshes.clear();
	m_blended_meshes.clear();
	m_transform_cache.clear();

	{
		ZoneScopedNC("Prepare render data", 0xaa4455);
		// render opaque submeshes first and put masked and blended submeshes in respective queues
		for(unsigned model_idx = 0; model_idx < m_scene->models.size(); ++model_idx) {
			const Model& model = m_scene->models[model_idx];

			for(unsigned model_mesh_idx = 0; model_mesh_idx < model.mesh_count; ++model_mesh_idx) {
				const auto& shaded_mesh = m_scene->shaded_meshes[model.shaded_meshes[model_mesh_idx]];
				auto submesh_global_idx = shaded_mesh.mesh;
				const model::Mesh& submesh = m_scene->submeshes[submesh_global_idx];
				const Material& material   = m_scene->materials[shaded_mesh.material];

				auto final_transform_mat = model.transform.mat * shaded_mesh.transform.mat;
				auto inv_final_transform_mat = shaded_mesh.transform.inv_mat * model.transform.inv_mat; // (AB)^-1 = B^-1 * A^-1
				auto submesh_bbox = bbox3::transformed(submesh.bbox, final_transform_mat);

				m_transform_cache.push_back({final_transform_mat, inv_final_transform_mat, submesh_bbox});
				uint32_t transform_idx = m_transform_cache.size()-1;

				if(material.alpha_mode() == Texture::AlphaMode::Mask) {
					m_masked_meshes.push_back(ModelMeshIdx{model_idx, model.shaded_meshes[model_mesh_idx], transform_idx});
					continue;
				}
				if(material.alpha_mode() == Texture::AlphaMode::Blend) {
					m_blended_meshes.push_back(ModelMeshIdx{model_idx, model.shaded_meshes[model_mesh_idx], transform_idx});
					continue;
				}
				m_opaque_meshes.push_back(ModelMeshIdx{model_idx, model.shaded_meshes[model_mesh_idx], transform_idx});
			}

			if (draw_model_bboxes) {
				auto mesh_bbox = bbox3::transformed(model.bbox, model.transform.mat);
				dd::aabb(mesh_bbox.min(), mesh_bbox.max(), dd::colors::Purple);
			}
		}

	}

	TracyGpuZone("draw_generic");
	RC_DEBUG_GROUP("draw generic");

	m_perfquery.begin();

	if (do_shadow_mapping) {
		if (enable_directional_shadows && m_scene->directional_light.color_intensity.w > 0.0f)
			draw_directional_shadow();

		if (enable_point_shadows || enable_spot_shadows) {

			auto data = begin_draw_light_shadows();

			if (enable_point_shadows)
				draw_point_shadow(data);

			if (enable_spot_shadows)
				draw_spot_shadow(data);

			end_draw_light_shadows();
		}
	}

	glClearDepthf(0.0f);
	glDepthFunc(GL_GREATER);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// set out framebuffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, *m_backbuffer_fbo);
	auto render_size = _render_size();
	glViewport(0, 0, render_size.x, render_size.y);

	glClear(GL_DEPTH_BUFFER_BIT); // NOTE: no need to clear color attachment bacause skybox will be drawn over it anyway

	set_uniforms();

	glUseProgram(*m_shader);

	if (do_shadow_mapping) {
		// bind shadow map texture
		glBindTextureUnit(32, *m_shadowmap_depth_to);
		glBindTextureUnit(36, *m_spot_shadow_depth_to);
		glBindTextureUnit(37, *m_point_shadow_depth_to);
	}

	int64_t num_point_lights = 0;
	int64_t num_spot_lights = 0;
	int64_t num_drawcalls = 0;

	auto render_mesh_by_index = [this, &num_point_lights, &num_spot_lights, &num_drawcalls](const ModelMeshIdx& idx, const zcm::vec3& bbox_color) {
		const MeshTransform& transform = m_transform_cache[idx.transform_idx];
		if(m_scene->main_camera.frustum.bbox_culled(transform.transformed_bbox))
			return;

		const auto& shaded_mesh = m_scene->shaded_meshes[idx.submesh_idx];
		const model::Mesh& submesh = m_scene->submeshes[shaded_mesh.mesh];
		const Material& material   = m_scene->materials[shaded_mesh.material];

		unif::m4(*m_shader, "model", transform.mat);
		unif::m3(*m_shader, "normal_matrix", zcm::transpose(zcm::mat3{transform.inv_mat}));

		num_point_lights += process_point_lights<MaxLights>(m_scene->point_lights, m_scene->main_camera.frustum, transform.transformed_bbox, *m_shader);
		num_spot_lights += process_spot_lights<MaxLights>(m_scene->spot_lights, m_scene->main_camera.frustum, transform.transformed_bbox, *m_shader);
		++num_drawcalls;
		render_generic(submesh, material, *m_shader);

		if(draw_mesh_bboxes)
			dd::aabb(transform.transformed_bbox.min(), transform.transformed_bbox.max(), bbox_color);
	};

	{
		RC_DEBUG_GROUP("opaque meshes");
		TracyGpuZoneC("draw_opaque", 0xaaaaaa);
		ZoneScopedN("draw_opaque");
		for(const auto& idx : m_opaque_meshes) {
			render_mesh_by_index(idx, dd::colors::White);
		}
	}



	if(MSAASampleCount > 1) {
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	} else {
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	}
	{
		RC_DEBUG_GROUP("masked meshes");
		TracyGpuZoneC("draw_masked", 0xaa4444);
		ZoneScopedN("draw_masked");
		for(const auto& idx : m_masked_meshes) {
			render_mesh_by_index(idx, dd::colors::Red);

		}
	}

	if(MSAASampleCount > 1) {
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	}

	TracyPlot("% unculled draws", (float)rc::math::percent(size_t(num_drawcalls), m_masked_meshes.size() + m_opaque_meshes.size() + m_blended_meshes.size()));
	TracyPlotConfig("% unculled draws", tracy::PlotFormatType::Percentage);

	TracyPlot("Point lights total", num_point_lights);
	TracyPlot("Spot lights total", num_spot_lights);

	const auto& frustum = m_scene->main_camera.frustum;
	if(frustum.state & Frustum::ShowWireframe) {
		frustum.draw_debug();
	}

	glEnable(GL_CULL_FACE);
	draw_skybox();

	dd::flush();

	if (do_bloom) {
		bloom_pass();
	}

	tonemap_pass();

	if (selected_fsr_preset != 0) {
		upscale_pass();
		glBlitNamedFramebuffer(*m_fsr_pass2_rcas_fbo, 0,
				       0, 0,
				       m_window_width, m_window_width,
				       0, 0,
				       m_window_width, m_window_width,
				       GL_COLOR_BUFFER_BIT, GL_NEAREST);
	} else {
		auto render_size = _render_size();
		glBlitNamedFramebuffer(*m_ldr_tonemap_fbo, 0,
		                       0, 0, render_size.x, render_size.y,
		                       0, 0, m_window_width, m_window_height,
		                       GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_per_frame.finish();
	m_light_per_frame.finish();
	m_tonemap_uniform.finish();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
	m_perfquery.end();
}

void Renderer::draw_gui(Renderer::RenderParams& params)
{
	ZoneScoped;
#if 0
	ImGui::Begin("BRDF LUT");
	const auto uv0 = ImVec2(0.0f, 1.0f);
	const auto uv1 = ImVec2(1.0f, 0.0f);
	ImGui::Image((ImTextureID)(uintptr_t)(*m_brdf_lut_to), ImVec2(256, 256), uv0, uv1);
	ImGui::End();
#endif

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + zcm::vec2(5, 5), ImGuiCond_Always);

	ImGui::SetNextWindowSize(ImVec2(30 * ImGui::GetFontSize(), 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.1f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::Begin("Profile", nullptr,
		ImGuiWindowFlags_NoMove
		|ImGuiWindowFlags_NoResize
		|ImGuiWindowFlags_NoSavedSettings
		|ImGuiWindowFlags_NoTitleBar
		|ImGuiWindowFlags_NoScrollbar
		|ImGuiWindowFlags_NoCollapse
		|ImGuiWindowFlags_NoDocking);

	ImGui::PushItemWidth(-1.0f);

	{
		ZoneNamedN(collect, "perfquery collect", true);
		TracyGpuZone("perfquery collect");

		m_perfquery.collect();

	}

	char avgbuf[64];
	snprintf(avgbuf, std::size(avgbuf),
	         "avg: %5.2f ms / %5.1f fps, last: %5.2f ms, qn: %d",
	         m_perfquery.time_avg,
	         1000.0f / std::max(m_perfquery.time_avg, 0.01f), m_perfquery.time_last, m_perfquery.query_num);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.00f, 0.00f, 0.00f, 0.50f));
	ImGui::PlotLines("", m_perfquery.times, std::size(m_perfquery.times), 0, avgbuf, 0.0f, 20.0f, ImVec2(0, 3 * ImGui::GetFontSize()));
	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.00f, 0.00f, 0.50f));
	if (ImGui::Button("Renderer")) {
		window_shown = !window_shown;
	}
	ImGui::SameLine();
	if (ImGui::Button("Scene")) {
		m_scene->window_shown = !m_scene->window_shown;
	}
	ImGui::PopStyleColor();

	ImGui::PopItemWidth();
	ImGui::End();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();

	if (!window_shown) {
		return;
	}

	if(!ImGui::Begin("Renderer", &window_shown)) {
		ImGui::End();
		return;
	}
	const char* labels[] = {"Disabled", "2x", "4x", "8x", "16x"};

	ImGui::Combo("MSAA", &desired_msaa_level, labels, std::size(labels));
	//show_help_tooltip("Multi-Sample Antialiasing\n\nValues > 4x may be unsupported on some setups.");

	const char* labels_fsr[] = {"Off", "Ultra Quality", "Quality", "Balanced", "Performance"};
	if (ImGui::Combo("FSR", &selected_fsr_preset, labels_fsr, std::size(labels_fsr))) {
		switch (selected_fsr_preset) {
			case 0: desired_render_scale = 1.0f; break;
			case 1: desired_render_scale = 0.77f; break;
			case 2: desired_render_scale = 0.67f; break;
			case 3: desired_render_scale = 0.59f; break;
			case 4: desired_render_scale = 0.50; break;
		}
	}

	if (selected_fsr_preset != 0) {
		ImGui::SliderFloat("FSR sharpening", &fsr_sharpening, 0.0f, 1.0f);
		ImGui::SliderFloat("FSR filmgrain", &fsr_filmgrain, 0.0f, 1.0f);
	}

	ImGui::BeginDisabled(selected_fsr_preset != 0);
	ImGui::SliderFloat("Resolution scale", &desired_render_scale, 0.1f, 1.0f, "%.1f");
	ImGui::EndDisabled();


	ImGui::Separator();
	auto render_size = _render_size();
	int width = render_size.x, height = render_size.y;
	ImGui::Text("render:  %d x %d", width, height);
	ImGui::Text("display: %d x %d", m_window_width, m_window_height);


	ImGui::Separator();


	ImGui::Checkbox("VSYNC", &params.vsync);
	ImGui::SameLine();
	ImGui::Checkbox("Allow tearing", &params.vsync_tearing);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, !params.vsync ? 1.0f : 0.3f);
	ImGui::Checkbox("Lock FPS", params.lock_fps);
	ImGui::SameLine();
	ImGui::SliderFloat("Target FPS", params.target_fps, 1.0f, 240.0f);
	ImGui::PopStyleVar();

	ImGui::Spacing();
	ImGui::Checkbox("Bloom", &do_bloom);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, do_bloom ? 1.0f : 0.3f);
	ImGui::SliderFloat("Bloom threshold", &bloom_threshold, 0.05f, 10.0f);
	ImGui::SliderFloat("Bloom strength", &bloom_strength, 0.05f, 2.0f);
	ImGui::PopStyleVar();
	ImGui::Spacing();

	ImGui::Checkbox("Shadows", &do_shadow_mapping);
	ImGui::SameLine();
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, do_shadow_mapping ? 1.0f : 0.3f);
	ImGui::Checkbox("Directional", &enable_directional_shadows);
	ImGui::SameLine();
	ImGui::Checkbox("Point", &enable_point_shadows);
	ImGui::SameLine();
	ImGui::Checkbox("Spot", &enable_spot_shadows);
	ImGui::Checkbox("Shadow caching", &enable_shadow_caching);
	ImGui::PopStyleVar();
	ImGui::Spacing();


	ImGui::Checkbox("Show submesh bboxes", &draw_mesh_bboxes);
	ImGui::SameLine();
	ImGui::Checkbox("Show model bboxes", &draw_model_bboxes);

	const char* labels_cube[] = {"Sky", "Diffuse Irradiance", "Specular Reflectance"};
	ImGui::Combo("Cubemap", &selected_cubemap, labels_cube, std::size(labels_cube));
	if (selected_cubemap == 2) {
		ImGui::SliderInt("Roughness level", &cubemap_mip_level, 0, 10);
	}
	ImGui::End();
}

void Renderer::save_hdr_backbuffer(std::string_view path)
{
	auto render_size = _render_size();
	glBlitNamedFramebuffer(*m_backbuffer_fbo,
	                       *m_backbuffer_resolve_fbo,
	                       0, 0, render_size.x, render_size.y,
	                       0, 0, render_size.x, render_size.y,
	                       GL_COLOR_BUFFER_BIT,
	                       GL_NEAREST);
	util::gl_save_hdr_texture(*m_backbuffer_resolve_color_to, path);
}

void Renderer::draw_skybox()
{
	ZoneScoped;
	RC_DEBUG_GROUP("skybox");
	glDepthFunc(GL_GEQUAL);
	auto view = make_view(m_scene->main_camera.state);
	auto projection = make_projection(m_scene->main_camera.state);

	int level = selected_cubemap == 2 ? cubemap_mip_level : 0;

	switch (selected_cubemap) {
	case 0:
		Cubemap::draw(m_scene->cubemap, view, projection, level);
		break;
	case 1:
		Cubemap::draw(m_scene->cubemap_diffuse_irradiance, view, projection, level);
		break;
	case 2:
		Cubemap::draw(m_scene->cubemap_specular_environment, view, projection, level);
		break;
	default:
		assert(false);
		unreachable();
	}
	glDepthFunc(GL_GREATER);
}





