#include <rendercat/common.hpp>
#include <rendercat/renderer.hpp>
#include <rendercat/scene.hpp>
#include <rendercat/uniform.hpp>
#include <iostream>
#include <cstdio>
#include <string>
#include <imgui.hpp>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;

Renderer::Renderer(Scene * s) : m_scene(s)
{
	assert(s != nullptr);

	m_shader = m_shader_set.load_program({"generic.vert", "generic.frag"});
	m_cubemap_shader = m_shader_set.load_program({"cubemap.vert", "cubemap.frag"});
	m_hdr_shader = m_shader_set.load_program({"hdr.vert", "hdr.frag"});

	glEnable(GL_FRAMEBUFFER_SRGB);

	glGetIntegerv(GL_MAX_SAMPLES, &MSAASampleCountMax);
	std::cout << "[renderer] max MSAA samples: " << MSAASampleCountMax << '\n';
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_backbuffer_color_to);
	glDeleteTextures(1, &m_backbuffer_depth_to);
	glDeleteFramebuffers(1, &m_backbuffer_fbo);
}

void Renderer::resize(uint32_t width, uint32_t height, bool force)
{
	if(width < 2 || height < 2)
		return;

	if(!force && (width == m_window_width || height == m_window_height))
		return;

	m_backbuffer_scale = m_scene->desired_render_scale;
	msaa_level = m_scene->desired_msaa_level;

	// NOTE: for some reason AMD driver accepts 0 samples, but it's wrong
	MSAASampleCount = glm::clamp((int)std::exp2(msaa_level), 1, MSAASampleCountMax);

	m_window_width = width;
	m_window_height = height;

	m_backbuffer_width  = width * m_backbuffer_scale;
	m_backbuffer_height = height * m_backbuffer_scale;

	// reinitialize multisampled color texture object
	glDeleteTextures(1, &m_backbuffer_color_to);
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_backbuffer_color_to);

	glTextureStorage2DMultisample(m_backbuffer_color_to,
	                              MSAASampleCount,
	                              GL_RGBA16F,
	                              m_backbuffer_width,
	                              m_backbuffer_height,
	                              GL_FALSE);


	// reinitialize multisampled depth texture object
	glDeleteTextures(1, &m_backbuffer_depth_to);
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_backbuffer_depth_to);

	// TODO: research if GL_DEPTH24_STENCIL8 is viable here
	glTextureStorage2DMultisample(m_backbuffer_depth_to,
	                              MSAASampleCount,
	                              GL_DEPTH_COMPONENT32F,
	                              m_backbuffer_width,
	                              m_backbuffer_height,
	                              GL_FALSE);


	// reinitialize framebuffer
	glDeleteFramebuffers(1, &m_backbuffer_fbo);
	glCreateFramebuffers(1, &m_backbuffer_fbo);

	// attach texture objects
	glNamedFramebufferTexture(m_backbuffer_fbo,
	                          GL_COLOR_ATTACHMENT0,
	                          m_backbuffer_color_to,
	                          0);
	glNamedFramebufferTexture(m_backbuffer_fbo,
	                          GL_DEPTH_ATTACHMENT,
	                          m_backbuffer_depth_to,
	                          0);


	GLenum fboStatus = glCheckNamedFramebufferStatus(m_backbuffer_fbo, GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("[renderer] could not resize backbuffer!");
	}

	m_scene->main_camera.update_projection(float(m_backbuffer_width) / (float)m_backbuffer_height);
	std::cout << "[renderer] framebuffer resized to " << m_backbuffer_width << " x " << m_backbuffer_height << std::endl;
}

static void renderQuad()
{
	static GLuint vao;
	static GLuint vbo;
	if (unlikely(vao == 0)) {
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 1.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
		};
		glCreateVertexArrays(1, &vao);
		glCreateBuffers(1, &vbo);

		glNamedBufferStorage(vbo, sizeof(quadVertices), &quadVertices, GL_NONE_BIT);

		// set up VBO: binding index, vbo, offset, stride
		glVertexArrayVertexBuffer(vao, 0, vbo, 0, 5*sizeof(float));

		glEnableVertexArrayAttrib(vao, 0);
		glEnableVertexArrayAttrib(vao, 1);

		// specify attrib format: attrib idx, element count, format, normalized, relative offset
		glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float));

		// assign VBOs to attributes
		glVertexArrayAttribBinding(vao, 0, 0);
		glVertexArrayAttribBinding(vao, 1, 0);
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
	unif::m4(shader, "proj_view", m_scene->main_camera.projection * m_scene->main_camera.view);
	unif::v3(shader, "viewPos",   m_scene->main_camera.pos);
	unif::v3(shader, "directional_light.direction", m_scene->directional_light.direction);
	unif::v3(shader, "directional_light.ambient",   m_scene->directional_light.ambient);
	unif::v3(shader, "directional_light.diffuse",   m_scene->directional_light.diffuse);
	unif::v3(shader, "directional_light.specular",  m_scene->directional_light.specular);

	for(unsigned i = 0; i < m_scene->point_lights.size() && i < MaxLights;++i) {
		const auto& light = m_scene->point_lights[i];
		unif::v3(shader, indexed_uniform("point_light", ".position",  i),  light.position());
		unif::v3(shader, indexed_uniform("point_light", ".ambient",   i),  light.ambient());
		unif::v3(shader, indexed_uniform("point_light", ".diffuse",   i),  light.diffuse());
		unif::v3(shader, indexed_uniform("point_light", ".specular",  i),  light.specular());
		unif::f1(shader, indexed_uniform("point_light", ".radius",    i),  light.radius());
		unif::f1(shader, indexed_uniform("point_light", ".intensity", i),  light.intensity());
	}

	for(unsigned i = 0; i < m_scene->spot_lights.size() && i < MaxLights;++i) {
		const auto& light = m_scene->spot_lights[i];
		unif::v3(shader, indexed_uniform("spot_light", ".direction", i),  light.direction());
		unif::v3(shader, indexed_uniform("spot_light", ".position",  i),  light.position());
		unif::v3(shader, indexed_uniform("spot_light", ".ambient",   i),  light.ambient());
		unif::v3(shader, indexed_uniform("spot_light", ".diffuse",   i),  light.diffuse());
		unif::v3(shader, indexed_uniform("spot_light", ".specular",  i),  light.specular());
		unif::f1(shader, indexed_uniform("spot_light", ".radius",    i),  light.radius());
		unif::f1(shader, indexed_uniform("spot_light", ".intensity", i),  light.intensity());
		unif::f1(shader, indexed_uniform("spot_light", ".angle_scale", i),light.angle_scale());
		unif::f1(shader, indexed_uniform("spot_light", ".angle_offset",i),light.angle_offset());
	}
}

void Renderer::draw()
{
	if(unlikely(!m_shader)) return;
	m_shader_set.check_updates();

	if(unlikely(m_scene->desired_render_scale != m_backbuffer_scale || m_scene->desired_msaa_level != msaa_level))
		resize(m_window_width, m_window_height, true);

	m_perfquery.begin();

	glClearDepth(0.0f);
	glDepthFunc(GL_GREATER);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// set out framebuffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, m_backbuffer_fbo);
	glViewport(0, 0, m_backbuffer_width, m_backbuffer_height);
	glClear(GL_DEPTH_BUFFER_BIT); // NOTE: no need to clear color attachment bacause skybox will be drawn over it anyway

	glUseProgram(*m_shader);
	set_uniforms(*m_shader);

	for(unsigned model_idx = 0; model_idx < m_scene->models.size(); ++model_idx) {
		const Model& model = m_scene->models[model_idx];

		unif::m4(*m_shader, "model", model.transform);
		unif::m3(*m_shader, "normal_matrix", glm::transpose(model.inv_transform));

		for(unsigned submesh_idx = 0; submesh_idx < model.submesh_count; ++submesh_idx) {
			const model::mesh& submesh = m_scene->submeshes[model.submeshes[submesh_idx]];
			const Material& material = m_scene->materials[model.materials[submesh_idx]];

			if(material.flags & Material::FaceCullingDisabled) {
				glDisable(GL_CULL_FACE);
			} else {
				glEnable(GL_CULL_FACE);
			}

			auto submesh_aabb = submesh.aabb;
			submesh_aabb.translate(model.transform[3]);
			int point_light_count = 0;
			for(unsigned i = 0; i < m_scene->point_lights.size() && i < MaxLights; ++i) {
				const auto& light = m_scene->point_lights[i];
				if(!light.enabled)
					continue;

				// TODO: implement per-light AABB cutoff to prevent light leaking
				if(submesh_aabb.intersects_sphere(light.position(), light.radius())) {
					auto dist = glm::length(light.position() - submesh_aabb.closest_point(light.position()));
					if(light.distance_attenuation(dist) > 0.0f) {
						unif::i1(*m_shader, indexed_uniform("point_light_indices", "", point_light_count++), i);
					}
				}
			}
			unif::i1(*m_shader, "num_point_lights", point_light_count);

			int spot_light_count = 0;
			for(unsigned i = 0; i < m_scene->spot_lights.size() && i < MaxLights; ++i) {
				const auto& light = m_scene->spot_lights[i];
				if(!light.enabled)
					continue;

				// TODO: implement cone-AABB collision check
				if(submesh_aabb.intersects_sphere(light.position(), light.radius())) {
					auto dist = glm::length(light.position() - submesh_aabb.closest_point(light.position()));
					if(light.distance_attenuation(dist) > 0.0f) {
						unif::i1(*m_shader, indexed_uniform("spot_light_indices", "", spot_light_count++), i);
					}
				}
			}
			unif::i1(*m_shader, "num_spot_lights", spot_light_count);

			material.bind(*m_shader);
			glBindVertexArray(submesh.vao);
			assert(submesh.index_type == GL_UNSIGNED_INT || submesh.index_type == GL_UNSIGNED_SHORT);
			glDrawElements(GL_TRIANGLES, submesh.numverts, submesh.index_type, nullptr);
		}
	}

	glEnable(GL_CULL_FACE);

	draw_skybox();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_window_width, m_window_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(*m_hdr_shader);
	glBindTextureUnit(0, m_backbuffer_color_to);
	unif::f1(*m_hdr_shader, "exposure", m_scene->main_camera.exposure);
	unif::i2(*m_hdr_shader, "texture_size", m_backbuffer_width, m_backbuffer_height);
	unif::i1(*m_hdr_shader, "sample_count", MSAASampleCount);
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
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	ImGui::Begin("Profile", nullptr, ImVec2(350, 40), 0.0f,
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
	         "GPU avg: %5.2f ms (%4.1f fps), last: %5.2f ms",
	         m_perfquery.time_avg,
	         1000.0f / std::max(m_perfquery.time_avg, 0.01f), m_perfquery.time_last);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.00f, 0.00f, 0.00f, 0.50f));
	ImGui::PlotLines("", m_perfquery.times, std::size(m_perfquery.times), 0, avgbuf, 0.0f, 20.0f, ImVec2(0,40));
	ImGui::PopStyleColor();

	ImGui::PopItemWidth();
	ImGui::End();
	ImGui::PopStyleVar(2);
}

void Renderer::draw_skybox()
{
	glDepthFunc(GL_GEQUAL);
	m_scene->cubemap.draw(*m_cubemap_shader, m_scene->main_camera.view, m_scene->main_camera.projection);
	glDepthFunc(GL_GREATER);
}
