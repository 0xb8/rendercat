#include <rendercat/common.hpp>
#include <rendercat/renderer.hpp>
#include <rendercat/scene.hpp>
#include <rendercat/uniform.hpp>
#include <rendercat/util/gl_screenshot.hpp>
#include <rendercat/util/turbo_colormap.hpp>
#include <fmt/core.h>
#include <string>
#include <imgui.h>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding-aux/Meta.h>

#include <debug_draw.hpp>
#include <tracy/Tracy.hpp>


#include <zcm/mat3.hpp>
#include <zcm/exponential.hpp>
#include <zcm/matrix_transform.hpp>
#include <zcm/type_ptr.hpp>

using namespace gl45core;
#define GL_TIMESTAMP gl45core::GL_TIMESTAMP
#include <tracy/TracyOpenGL.hpp>

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

Renderer::Renderer(Scene * s) : m_scene(s)
{
	assert(s != nullptr);

	m_shader = m_shader_set.load_program({"generic.vert", "generic.frag"});
	m_hdr_shader = m_shader_set.load_program({"fullscreen_quad.vert", "hdr.frag"});

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

	dd::initialize(&debug_draw_ctx);
	init_shadow();
	init_brdf();
	init_colormap();
}

void Renderer::init_shadow()
{
	m_shadow_shader = m_shader_set.load_program({"shadow_mapping.vert", "shadow_mapping.frag"});
	m_shadow_masked_shader = m_shader_set.load_program({"shadow_mapping.vert", "shadow_mapping.frag"},
	                                                   {{"ALPHA_MASKED"}});

	// create texture for directional light shadowmap
	glCreateTextures(GL_TEXTURE_2D, 1, m_shadowmap_depth_to.get());
	glTextureStorage2D(*m_shadowmap_depth_to, 1, GL_DEPTH_COMPONENT32F, ShadowMapWidth, ShadowMapHeight);
	float borderColor[] = {0.0f, 0.0f, 0.0f, 1.0f };
	glTextureParameterfv(*m_shadowmap_depth_to, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTextureParameteri(*m_shadowmap_depth_to, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTextureParameteri(*m_shadowmap_depth_to, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	glTextureParameteri(*m_shadowmap_depth_to, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(*m_shadowmap_depth_to, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glCreateFramebuffers(1, m_shadowmap_fbo.get());
	glNamedFramebufferTexture(*m_shadowmap_fbo, GL_DEPTH_ATTACHMENT, *m_shadowmap_depth_to, 0);

	if (glCheckNamedFramebufferStatus(*m_shadowmap_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not init shadow framebuffer!");
		std::fflush(stderr);
		return;
	}

	// create texture array for spot light shadowmaps
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, m_spot_shadow_depth_to.get());
	glTextureStorage3D(*m_spot_shadow_depth_to, 1, GL_DEPTH_COMPONENT16, PointShadowWidth, PointShadowHeight, MaxLights);
	glTextureParameterfv(*m_spot_shadow_depth_to, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTextureParameteri(*m_spot_shadow_depth_to, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTextureParameteri(*m_spot_shadow_depth_to, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	glTextureParameteri(*m_spot_shadow_depth_to, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(*m_spot_shadow_depth_to, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glCreateFramebuffers(1, m_spot_shadow_fbo.get());
	glNamedFramebufferTextureLayer(*m_spot_shadow_fbo, GL_DEPTH_ATTACHMENT, *m_spot_shadow_depth_to, 0, 0);

	if (glCheckNamedFramebufferStatus(*m_spot_shadow_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not init point shadow framebuffer!");
		std::fflush(stderr);
		return;
	}

}

void Renderer::init_brdf()
{
	m_brdf_shader = m_shader_set.load_program({"brdf_lut.comp"});
	if (!m_brdf_shader) {
		fmt::print(stderr, "[renderer] could not init BRDF LUT compute shader!");
		std::fflush(stderr);
		return;
	}

	int size = 256;

	glCreateTextures(GL_TEXTURE_2D, 1, m_brdf_lut_to.get());
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
	glCreateTextures(GL_TEXTURE_1D, 1, m_turbo_colormap_to.get());
	glTextureStorage1D(*m_turbo_colormap_to, 1, GL_RGB32F, 256);
	glTextureSubImage1D(*m_turbo_colormap_to, 0, 0, 256, GL_RGB, GL_FLOAT, &rc::util::turbo_srgb_floats);
}

void Renderer::resize(uint32_t width, uint32_t height, float device_pixel_ratio, bool force)
{
	if(!force && (width == m_window_width && height == m_window_height))
		return;

	if(width < 2 || height < 2)
		return;

	ZoneScoped;

	m_device_pixel_ratio = device_pixel_ratio;

	if(unlikely(width > (GLuint)max_framebuffer_width || height > (GLuint)max_framebuffer_height))
		throw std::runtime_error("framebuffer resize failed: specified size too big");

	// NOTE: for some reason AMD driver accepts 0 samples, but it's wrong
	int samples = zcm::clamp((int)zcm::exp2(desired_msaa_level), 1, MSAASampleCountMax);
	uint32_t backbuffer_width = width * desired_render_scale;
	uint32_t backbuffer_height = height * desired_render_scale;

	// reinitialize multisampled color texture object
	rc::texture_handle color_to;
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, color_to.get());
	glTextureStorage2DMultisample(*color_to,
				      samples,
				      GL_RGBA16F,
				      backbuffer_width,
				      backbuffer_height,
				      GL_FALSE);

	// reinitialize multisampled depth texture object
	rc::texture_handle depth_to;
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, depth_to.get());
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
	glTextureStorage2D(*resolve_to, 1, GL_RGBA16F, backbuffer_width, backbuffer_height);

	rc::framebuffer_handle resolve_fbo;
	glCreateFramebuffers(1, resolve_fbo.get());
	glNamedFramebufferTexture(*resolve_fbo, GL_COLOR_ATTACHMENT0, *resolve_to, 0);

	if (glCheckNamedFramebufferStatus(*resolve_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fmt::print(stderr, "[renderer] could not resize resolve backbuffer!");
		std::fflush(stderr);
		return;
	}

	m_backbuffer_fbo = std::move(fbo);
	m_backbuffer_color_to = std::move(color_to);
	m_backbuffer_depth_to = std::move(depth_to);
	m_backbuffer_resolve_fbo = std::move(resolve_fbo);
	m_backbuffer_resolve_color_to = std::move(resolve_to);
	m_backbuffer_scale = desired_render_scale;
	msaa_level = desired_msaa_level;
	MSAASampleCount = samples;
	m_window_width = width;
	m_window_height = height;
	m_backbuffer_width = backbuffer_width;
	m_backbuffer_height = backbuffer_height;

	m_scene->main_camera.state.aspect = (float(m_backbuffer_width) / (float)m_backbuffer_height);
	debug_draw_ctx.window_size = zcm::vec2{(float)backbuffer_width, (float)backbuffer_height};
	fmt::print(stderr, "[renderer] framebuffer resized to {}x{} {}\n", m_backbuffer_width, m_backbuffer_height, force ? "(forced)" : "");
	std::fflush(stderr);
}

void Renderer::clear_screen()
{
	TracyGpuZone("clear screen");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_window_width, m_window_height);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

static void renderQuad()
{
	static GLuint vao;
	if (unlikely(vao == 0)) {
		glCreateVertexArrays(1, &vao);
	}
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

static const char* indexed_uniform(std::string_view array, std::string_view name, unsigned idx)
{
	static char buf[128] = {0};
	snprintf(buf, std::size(buf), "%s[%d]%s", array.data(), idx, name.data());
	return &buf[0];
}

void Renderer::set_uniforms()
{
	ZoneScoped;
	m_scene->main_camera.update();

	auto view = make_view(m_scene->main_camera.state);
	auto projection = make_projection(m_scene->main_camera.state);
	auto proj_view = projection * view;
	debug_draw_ctx.mvpMatrix = proj_view;

	m_per_frame.next();
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

	per_frame->dir_fog.inscattering_color     = m_scene->fog.inscattering_color;
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
	per_frame->shadows_enabled = do_shadow_mapping;

	m_per_frame.flush();

	// ------

	m_scene->cubemap_diffuse_irradiance.bind_to_unit(34);
	m_scene->cubemap_specular_environment.bind_to_unit(33);
	glBindTextureUnit(30, *m_turbo_colormap_to);
	glBindTextureUnit(31, *m_brdf_lut_to);
}

void Renderer::set_shadow_uniforms()
{
	// TODO: determine frustum size dynamically
	static float near_plane = -25.0f, far_plane = 25.0f;
	static zcm::vec4 params = zcm::vec4(-15.0f, 15.0f, -15.0f, 15.0f);
	auto pos = m_scene->directional_light.direction;

	zcm::mat4 lightProjection = zcm::orthoRH_ZO(params[0], params[1], params[2],params[3], near_plane, far_plane);
	auto lightView = zcm::lookAtRH(pos,
	                             zcm::vec3(0.0f, 0.0f, 0.0f),
	                             zcm::vec3(0.0f, 1.0f, 0.0f));

	auto proj_view = lightProjection * lightView;

	unif::m4(*m_shadow_shader, 0, proj_view);
	m_shadow_matrix = proj_view;
}

static int process_point_lights(const std::vector<PointLight>& point_lights,
                                 const Frustum& frustum,
                                 const bbox3& submesh_bbox,
                                 uint32_t shader)
{
	int point_light_count = 0;
	for(unsigned i = 0; i < point_lights.size() && i < Renderer::MaxLights; ++i) {
		const auto& light = point_lights[i];
		if(!(light.state & PointLight::Enabled))
			continue;
		if(frustum.sphere_culled(light.position(), light.radius()))
			continue;

		// TODO: implement per-light AABB cutoff to prevent light leaking
		if(bbox3::intersects_sphere(submesh_bbox, light.position(), light.radius()) != Intersection::Outside) {
			auto dist = zcm::length(light.position() - submesh_bbox.closest_point(light.position()));
			if(light.distance_attenuation(dist) > 0.0f) {
				unif::i1(shader, indexed_uniform("point_light_indices", "", point_light_count++), i);
			}
		}
	}
	unif::i1(shader, "num_point_lights", point_light_count);
	return point_light_count;
}

static int process_spot_lights(const std::vector<SpotLight>& spot_lights,
                                const Frustum& frustum,
                                const bbox3& submesh_bbox,
                                uint32_t shader)
{
	int spot_light_count = 0;
	for(unsigned i = 0; i < spot_lights.size() && i < Renderer::MaxLights; ++i) {
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
				unif::i1(shader, indexed_uniform("spot_light_indices", "", spot_light_count++), i);
			}
		}
	}
	unif::i1(shader, "num_spot_lights", spot_light_count);
	return spot_light_count;
}

static void submit_draw_call(const model::Mesh& submesh)
{
	glBindVertexArray(*submesh.vao);

	if (likely(submesh.index_type)) {

		assert(submesh.index_type == GL_UNSIGNED_INT
		       || submesh.index_type == GL_UNSIGNED_SHORT
		       || submesh.index_type == GL_UNSIGNED_BYTE);

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
	} else {
		glDrawArrays((GLenum)submesh.draw_mode, 0, submesh.numverts);
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


void Renderer::draw_shadow()
{
	ZoneScoped;
	TracyGpuZone("draw_shadow");
	glClearDepthf(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // TODO: use front face culling

	glBindFramebuffer(GL_FRAMEBUFFER, *m_shadowmap_fbo);
	glViewport(0,0, ShadowMapWidth, ShadowMapHeight);
	glClear(GL_DEPTH_BUFFER_BIT);

	set_shadow_uniforms();

	glUseProgram(*m_shadow_shader);

	for(unsigned model_idx = 0; model_idx < m_scene->models.size(); ++model_idx) {
		const Model& model = m_scene->models[model_idx];

		unif::m4(*m_shadow_shader, 1, model.transform.mat);

		// TODO: culling
		for(unsigned submesh_idx = 0; submesh_idx < model.mesh_count; ++submesh_idx) {
			const model::Mesh& submesh = m_scene->submeshes[m_scene->shaded_meshes[model.shaded_meshes[submesh_idx]].mesh];
			submit_draw_call(submesh);
		}
	}

	glUseProgram(0);
	glBindVertexArray(0);
}

void Renderer::draw_spot_shadow()
{
	ZoneScoped;
	TracyGpuZone("draw_spot_shadow");
	glClearDepthf(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // TODO: use front face culling

	glBindFramebuffer(GL_FRAMEBUFFER, *m_spot_shadow_fbo);
	glViewport(0,0, PointShadowWidth, PointShadowHeight);
	glClear(GL_DEPTH_BUFFER_BIT);

	m_light_per_frame.next();
	auto per_frame = m_light_per_frame.data();
	assert(per_frame);
	m_light_per_frame.bind(2);

	std::fill(std::begin(per_frame->shadow_indices), std::end(per_frame->shadow_indices), LightPerframeData::LightIndex{-1});

	int point_light_index = 0;
	for(auto&& light : m_scene->point_lights) {

		if(!(light.state & SpotLight::Enabled))
			continue;

		if(m_scene->main_camera.frustum.sphere_culled(light.position(), light.radius()))
			continue;

		++point_light_index;

		if(!(light.state & PointLight::ShowWireframe))
			continue;

		dd::sphere(light.position(), light.color(), 0.01f * light.radius(), 0, false);
		dd::sphere(light.position(), light.color(), light.radius());
	}

	int spot_light_index = 0;

	for(size_t scene_index = 0; scene_index < m_scene->spot_lights.size() && scene_index < MaxLights; ++scene_index) {
		const auto& light = m_scene->spot_lights[scene_index];



		if(m_scene->main_camera.frustum.sphere_culled(light.position(), light.radius()))
			continue;

		glNamedFramebufferTextureLayer(*m_spot_shadow_fbo, GL_DEPTH_ATTACHMENT, *m_spot_shadow_depth_to, 0, spot_light_index);
		if (glCheckNamedFramebufferStatus(*m_spot_shadow_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			fmt::print(stderr, "[renderer] could not update point shadow framebuffer!");
			std::fflush(stderr);
			return;
		}
		glClear(GL_DEPTH_BUFFER_BIT);
		glUseProgram(*m_shadow_shader);

		per_frame->shadow_indices[scene_index].index = spot_light_index;
		++spot_light_index;

		auto light_camera_state = CameraState{zcm::conjugate(light.orientation()), light.position()};
		light_camera_state.zfar = light.radius();
		light_camera_state.znear = 0.1f;
		light_camera_state.fov = light.angle_outer() * 1.9f;
		light_camera_state.aspect = 1.0f;
		light_camera_state.normalize();

		auto view_mat = make_view(light_camera_state);
		auto proj_mat = make_projection_non_reversed_z(light_camera_state);

		auto light_mat = proj_mat * view_mat;
		per_frame->spot_light_matrices[scene_index] = light_mat;

		unif::m4(*m_shadow_shader, 0, light_mat);

		auto frustum = Frustum();
		frustum.update(light_camera_state);

		for (const auto& idx : m_opaque_meshes) {
			const MeshTransform& transform = m_transform_cache[idx.transform_idx];
			if (frustum.bbox_culled(transform.transformed_bbox))
				continue;

			const auto& shaded_mesh = m_scene->shaded_meshes[idx.submesh_idx];
			const model::Mesh& submesh = m_scene->submeshes[shaded_mesh.mesh];

			unif::m4(*m_shadow_shader, 1, transform.mat);
			submit_draw_call(submesh);
		}

		glUseProgram(*m_shadow_masked_shader);
		unif::m4(*m_shadow_masked_shader, 0, light_mat);

		for (const auto& idx : m_masked_meshes) {
			const MeshTransform& transform = m_transform_cache[idx.transform_idx];
			if (frustum.bbox_culled(transform.transformed_bbox))
				continue;

			const auto& shaded_mesh = m_scene->shaded_meshes[idx.submesh_idx];
			const model::Mesh& submesh = m_scene->submeshes[shaded_mesh.mesh];
			const auto& material = m_scene->materials[shaded_mesh.material];

			unif::m4(*m_shadow_masked_shader, 1, transform.mat);
			material.bind(*m_shadow_masked_shader);
			submit_draw_call(submesh);
		}

		if(!(light.state & SpotLight::ShowWireframe))
			continue;

		dd::cone(light.position(),
		         zcm::normalize(-light.direction_vec()) * light.radius(),
		         light.color(),
		         light.angle_outer() * light.radius(), 0.0f);

		frustum.draw_debug();
	}

	TracyPlot("Visible spot lights", int64_t(spot_light_index));
	TracyPlot("Visible point lighs", int64_t(point_light_index));

	per_frame->num_visible_point_lights = point_light_index;
	per_frame->num_visible_spot_lights = spot_light_index;

	m_light_per_frame.flush();
	glUseProgram(0);
	glBindVertexArray(0);
}




void Renderer::draw()
{
	if(unlikely(!m_shader)) return;

	ZoneScoped;
	m_shader_set.check_updates();

	if(unlikely(desired_render_scale != m_backbuffer_scale || desired_msaa_level != msaa_level))
		resize(m_window_width, m_window_height, m_device_pixel_ratio, true);

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

	m_perfquery.begin();
	if(do_shadow_mapping) {
		// render shadow map
		draw_shadow();
	}

	if (do_shadow_mapping) {
		draw_spot_shadow();
	}

	glClearDepthf(0.0f);
	glDepthFunc(GL_GREATER);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// set out framebuffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, *m_backbuffer_fbo);
	glViewport(0, 0, m_backbuffer_width, m_backbuffer_height);
	glClear(GL_DEPTH_BUFFER_BIT); // NOTE: no need to clear color attachment bacause skybox will be drawn over it anyway

	set_uniforms();

	glUseProgram(*m_shader);

	if (do_shadow_mapping) {
		// bind shadow map texture
		glBindTextureUnit(32, *m_shadowmap_depth_to);
		glBindTextureUnit(36, *m_spot_shadow_depth_to);
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

		num_point_lights += process_point_lights(m_scene->point_lights, m_scene->main_camera.frustum, transform.transformed_bbox, *m_shader);
		num_spot_lights += process_spot_lights(m_scene->spot_lights, m_scene->main_camera.frustum, transform.transformed_bbox, *m_shader);
		++num_drawcalls;
		render_generic(submesh, material, *m_shader);

		if(draw_mesh_bboxes)
			dd::aabb(transform.transformed_bbox.min(), transform.transformed_bbox.max(), bbox_color);
	};

	{
		TracyGpuZoneC("draw_opaque", 0xaaaaaa);
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
		TracyGpuZoneC("draw_masked", 0xaa4444);
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



	{
		//	glBlitNamedFramebuffer(*m_backbuffer_fbo,
		//	                       *m_backbuffer_resolve_fbo,
		//	                       0, 0, m_backbuffer_width, m_backbuffer_height,
		//	                       0, 0, m_backbuffer_width, m_backbuffer_height,
		//	                       GL_COLOR_BUFFER_BIT,
		//	                       GL_NEAREST);

		TracyGpuNamedZone(rresolve, "resolve and tonemap", true);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, m_window_width, m_window_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(*m_hdr_shader);
		glBindTextureUnit(0, *m_backbuffer_color_to);
		unif::f1(*m_hdr_shader, 0, m_scene->main_camera.state.exposure);
		unif::i1(*m_hdr_shader, 1, MSAASampleCount);
		renderQuad();
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
	m_perfquery.end();
}

void Renderer::draw_gui()
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
		|ImGuiWindowFlags_NoCollapse);

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

	ImGui::PopItemWidth();
	ImGui::End();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();

	if(!ImGui::Begin("Renderer", &window_shown)) {
		ImGui::End();
		return;
	}
	const char* labels[] = {"Disabled", "2x", "4x", "8x", "16x"};

	ImGui::Combo("MSAA", &desired_msaa_level, labels, std::size(labels));
	//show_help_tooltip("Multi-Sample Antialiasing\n\nValues > 4x may be unsupported on some setups.");
	ImGui::SliderFloat("Resolution scale", &desired_render_scale, 0.1f, 2.0f, "%.1f");
	ImGui::Checkbox("Shadows", &do_shadow_mapping);
	ImGui::SameLine();
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
	glBlitNamedFramebuffer(*m_backbuffer_fbo,
	                       *m_backbuffer_resolve_fbo,
	                       0, 0, m_backbuffer_width, m_backbuffer_height,
	                       0, 0, m_backbuffer_width, m_backbuffer_height,
	                       GL_COLOR_BUFFER_BIT,
	                       GL_NEAREST);
	util::gl_save_hdr_texture(*m_backbuffer_resolve_color_to, path);
}

void Renderer::draw_skybox()
{
	ZoneScoped;
	glDepthFunc(GL_GEQUAL);
	auto view = make_view(m_scene->main_camera.state);
	auto projection = make_projection(m_scene->main_camera.state);

	int level = selected_cubemap == 2 ? cubemap_mip_level : 0;

	switch (selected_cubemap) {
	case 0:
		m_scene->cubemap.draw(view, projection, level);
		break;
	case 1:
		m_scene->cubemap_diffuse_irradiance.draw(view, projection, level);
		break;
	case 2:
		m_scene->cubemap_specular_environment.draw(view, projection, level);
		break;
	default:
		assert(false);
		unreachable();
	}
	glDepthFunc(GL_GREATER);
}



