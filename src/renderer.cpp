#include <rendercat/common.hpp>
#include <rendercat/renderer.hpp>
#include <rendercat/scene.hpp>
#include <rendercat/uniform.hpp>
#include <fmt/core.h>
#include <string>
#include <imgui.hpp>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding-aux/Meta.h>

#include <debug_draw.hpp>

#include <zcm/mat3.hpp>
#include <zcm/exponential.hpp>
#include <zcm/matrix_transform.hpp>

using namespace gl45core;
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
	m_cubemap_shader = m_shader_set.load_program({"cubemap.vert", "cubemap.frag"});
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
}

void Renderer::init_shadow()
{
	m_shadow_shader = m_shader_set.load_program({"shadow_mapping.vert", "shadow_mapping.frag"});

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
}


void Renderer::resize(uint32_t width, uint32_t height, bool force)
{
	if(!force && (width == m_window_width && height == m_window_height))
		return;

	if(width < 2 || height < 2)
		return;

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

void Renderer::set_uniforms(GLuint shader)
{
	m_scene->main_camera.update();


	auto view = make_view(m_scene->main_camera.state);
	auto projection = make_projection(m_scene->main_camera.state);
	auto view_projection = projection * view;


	debug_draw_ctx.mvpMatrix = view_projection;

	unif::m4(shader, "proj_view", view_projection);
	unif::v3(shader, "viewPos",   m_scene->main_camera.state.position);

	unif::v3(shader, "directional_light.direction", m_scene->directional_light.direction);
	unif::v3(shader, "directional_light.ambient",   m_scene->directional_light.ambient);
	unif::v3(shader, "directional_light.diffuse",   m_scene->directional_light.diffuse);
	unif::v3(shader, "directional_light.specular",  m_scene->directional_light.specular);

	unif::v4(shader, "directional_fog.inscattering_color",     m_scene->fog.inscattering_color);
	unif::v4(shader, "directional_fog.dir_inscattering_color", m_scene->fog.dir_inscattering_color);

	auto fog_dir_unif = zcm::vec4{-m_scene->directional_light.direction, m_scene->fog.dir_exponent};
	unif::v4(shader, "directional_fog.direction", fog_dir_unif);
	unif::f1(shader, "directional_fog.inscattering_density",   m_scene->fog.inscattering_density);
	unif::f1(shader, "directional_fog.extinction_density",     m_scene->fog.extinction_density);
	unif::b1(shader, "directional_fog.enabled",               (m_scene->fog.state & ExponentialDirectionalFog::Enabled));

	unif::i1(shader, "num_msaa_samples", MSAASampleCount);
	unif::b1(shader, "shadows_enabled", do_shadow_mapping);

	for(unsigned i = 0; i < m_scene->point_lights.size() && i < MaxLights;++i) {
		const auto& light = m_scene->point_lights[i];
		unif::v4(shader, indexed_uniform("point_light", ".position", i), zcm::vec4{light.position, light.radius});
		unif::v4(shader, indexed_uniform("point_light", ".color",    i), zcm::vec4{light.diffuse, light.luminous_intensity});
	}

	for(unsigned i = 0; i < m_scene->spot_lights.size() && i < MaxLights;++i) {
		const auto& light = m_scene->spot_lights[i];
		unif::v4(shader, indexed_uniform("spot_light", ".direction", i),   zcm::vec4{light.direction(), light.angle_scale()});
		unif::v4(shader, indexed_uniform("spot_light", ".position",  i),   zcm::vec4{light.position, light.radius});
		unif::v4(shader, indexed_uniform("spot_light", ".color",     i),   zcm::vec4{light.diffuse, light.luminous_intensity});
		unif::f1(shader, indexed_uniform("spot_light", ".angle_offset",i), light.angle_offset());
	}
}

zcm::mat4 Renderer::set_shadow_uniforms()
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

	unif::m4(*m_shadow_shader, "proj_view", proj_view);
	return proj_view;
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
		if(frustum.sphere_culled(light.position, light.radius))
			continue;

		// TODO: implement per-light AABB cutoff to prevent light leaking
		if(bbox3::intersects_sphere(submesh_bbox, light.position, light.radius) != Intersection::Outside) {
			auto dist = zcm::length(light.position - submesh_bbox.closest_point(light.position));
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

		if(frustum.sphere_culled(light.position, light.radius))
			continue;

		// TODO: implement cone-AABB collision check
		if(bbox3::intersects_sphere(submesh_bbox, light.position, light.radius) != Intersection::Outside) {
			auto dist = zcm::length(light.position - submesh_bbox.closest_point(light.position));
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
	if(material.double_sided) {
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
	glClearDepthf(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // TODO: use front face culling

	glBindFramebuffer(GL_FRAMEBUFFER, *m_shadowmap_fbo);
	glViewport(0,0, ShadowMapWidth, ShadowMapHeight);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(*m_shadow_shader);
	auto shadow_proj_view = set_shadow_uniforms();
	// set projection*view for main shader too
	unif::m4(*m_shader, "light_proj_view", shadow_proj_view);

	for(unsigned model_idx = 0; model_idx < m_scene->models.size(); ++model_idx) {
		const Model& model = m_scene->models[model_idx];

		unif::m4(*m_shadow_shader, "model", model.transform);

		// TODO: culling
		for(unsigned submesh_idx = 0; submesh_idx < model.submesh_count; ++submesh_idx) {
			const model::Mesh& submesh = m_scene->submeshes[model.submeshes[submesh_idx]];
			submit_draw_call(submesh);
		}
	}

	glUseProgram(0);
	glBindVertexArray(0);
}


void Renderer::draw()
{
	if(unlikely(!m_shader)) return;
	m_shader_set.check_updates();

	if(unlikely(desired_render_scale != m_backbuffer_scale || desired_msaa_level != msaa_level))
		resize(m_window_width, m_window_height, true);

	// clear masked and blended queue
	m_masked_meshes.clear();
	m_blended_meshes.clear();

	m_perfquery.begin();

	if(do_shadow_mapping) {
		// render shadow map
		draw_shadow();
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


	glUseProgram(*m_shader);
	set_uniforms(*m_shader);

	if (do_shadow_mapping) {
		// bind shadow map texture
		glBindTextureUnit(32, *m_shadowmap_depth_to);
	}

	// render opaque submeshes first and put masked and blended submeshes in respective queues
	for(unsigned model_idx = 0; model_idx < m_scene->models.size(); ++model_idx) {
		const Model& model = m_scene->models[model_idx];

		unif::m4(*m_shader, "model", model.transform);
		unif::m3(*m_shader, "normal_matrix", zcm::mat3{zcm::transpose(model.inv_transform)});

		for(unsigned submesh_idx = 0; submesh_idx < model.submesh_count; ++submesh_idx) {
			const model::Mesh& submesh = m_scene->submeshes[model.submeshes[submesh_idx]];
			const Material& material   = m_scene->materials[model.materials[submesh_idx]];

			if(material.alpha_mode == Texture::AlphaMode::Mask) {
				m_masked_meshes.push_back(ModelMeshIdx{model_idx, submesh_idx});
				continue;
			}
			if(material.alpha_mode == Texture::AlphaMode::Blend) {
				m_blended_meshes.push_back(ModelMeshIdx{model_idx, submesh_idx});
				continue;
			}

			auto submesh_bbox = bbox3::transformed(submesh.bbox, model.transform);
			if(m_scene->main_camera.frustum.bbox_culled(submesh_bbox))
				continue;

			process_point_lights(m_scene->point_lights, m_scene->main_camera.frustum, submesh_bbox, *m_shader);
			process_spot_lights(m_scene->spot_lights, m_scene->main_camera.frustum, submesh_bbox, *m_shader);
			render_generic(submesh, material, *m_shader);


			if(draw_mesh_bboxes)
				dd::aabb(submesh_bbox.min(), submesh_bbox.max(), dd::colors::White);
		}
	}

	if(MSAASampleCount > 1) {
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	} else {
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	}

	for(const auto& idx : m_masked_meshes) {
		const Model& model = m_scene->models[idx.model_idx];
		const model::Mesh& submesh = m_scene->submeshes[model.submeshes[idx.submesh_idx]];
		const Material& material   = m_scene->materials[model.materials[idx.submesh_idx]];

		auto submesh_bbox = bbox3::transformed(submesh.bbox, model.transform);
		if(m_scene->main_camera.frustum.bbox_culled(submesh_bbox))
			continue;

		unif::m4(*m_shader, "model", model.transform);
		unif::m3(*m_shader, "normal_matrix", zcm::mat3{zcm::transpose(model.inv_transform)});

		process_point_lights(m_scene->point_lights, m_scene->main_camera.frustum, submesh_bbox, *m_shader);
		process_spot_lights(m_scene->spot_lights, m_scene->main_camera.frustum, submesh_bbox, *m_shader);
		render_generic(submesh, material, *m_shader);

		if(draw_mesh_bboxes)
			dd::aabb(submesh_bbox.min(),submesh_bbox.max(), dd::colors::Aquamarine);

	}

	if(MSAASampleCount > 1) {
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	}

	for(auto&& light : m_scene->point_lights) {
		if(!(light.state & PointLight::ShowWireframe))
			continue;

		if(m_scene->main_camera.frustum.sphere_culled(light.position, light.radius))
			continue;

		dd::sphere(light.position, light.diffuse, 0.01f * light.radius, 0, false);
		dd::sphere(light.position, light.diffuse, light.radius);
	}

	for(auto&& light : m_scene->spot_lights) {
		if(!(light.state & SpotLight::ShowWireframe))
			continue;

		if(m_scene->main_camera.frustum.sphere_culled(light.position, light.radius))
			continue;

		dd::sphere(light.position, light.diffuse, 0.01f * light.radius, 0, false);
		dd::cone(light.position,
		         zcm::normalize(-light.direction()) * light.radius,
		         light.diffuse,
		         light.angle_outer() * light.radius, 0.0f);
		dd::cone(light.position,
		         zcm::normalize(-light.direction()) * light.radius,
		         light.diffuse,
		         light.angle_inner() * light.radius, 0.0f);
	}

	const auto& frustum = m_scene->main_camera.frustum;
	if(frustum.state & Frustum::ShowWireframe) {
		frustum.draw_debug();
	}

	glEnable(GL_CULL_FACE);
	draw_skybox();

	dd::flush();

//	glBlitNamedFramebuffer(*m_backbuffer_fbo,
//	                       *m_backbuffer_resolve_fbo,
//	                       0, 0, m_backbuffer_width, m_backbuffer_height,
//	                       0, 0, m_backbuffer_width, m_backbuffer_height,
//	                       GL_COLOR_BUFFER_BIT,
//	                       GL_NEAREST);


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_window_width, m_window_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(*m_hdr_shader);
	glBindTextureUnit(0, *m_backbuffer_color_to);
	unif::f1(*m_hdr_shader, 0, m_scene->main_camera.state.exposure);
	unif::i1(*m_hdr_shader, 1, MSAASampleCount);
	renderQuad();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
	m_perfquery.end();
}

void Renderer::draw_gui()
{
	ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(375, 40));
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

	m_perfquery.collect();

	char avgbuf[64];
	snprintf(avgbuf, std::size(avgbuf),
	         "avg: %5.2f ms / %5.1f fps, last: %5.2f ms, qn: %d",
	         m_perfquery.time_avg,
	         1000.0f / std::max(m_perfquery.time_avg, 0.01f), m_perfquery.time_last, m_perfquery.query_num);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.00f, 0.00f, 0.00f, 0.50f));
	ImGui::PlotLines("", m_perfquery.times, std::size(m_perfquery.times), 0, avgbuf, 0.0f, 20.0f, ImVec2(0,40));
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

	ImGui::Combo("MSAA", &desired_msaa_level, labels, 5);
	//show_help_tooltip("Multi-Sample Antialiasing\n\nValues > 4x may be unsupported on some setups.");
	ImGui::SliderFloat("Resolution scale", &desired_render_scale, 0.1f, 1.0f, "%.1f");
	ImGui::Checkbox("Shadows", &do_shadow_mapping);
	ImGui::Separator();
	ImGui::Checkbox("Show submesh bboxes", &draw_mesh_bboxes);
	ImGui::End();

}

void Renderer::draw_skybox()
{
	glDepthFunc(GL_GEQUAL);
	auto view = make_view(m_scene->main_camera.state);
	auto projection = make_projection(m_scene->main_camera.state);
	m_scene->cubemap.draw(*m_cubemap_shader, view, projection);
	glDepthFunc(GL_GREATER);
}

