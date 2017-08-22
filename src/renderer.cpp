#include <renderer.hpp>
#include <scene.hpp>
#include <iostream>
#include <cstdio>
#include <string>
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(Scene * s) : m_scene(s)
{
	assert(s != nullptr);

	m_shader = m_shader_set.load_program({"generic.vert", "generic.frag"});
	m_cubemap_shader = m_shader_set.load_program({"cubemap.vert", "cubemap.frag"});
	m_hdr_shader = m_shader_set.load_program({"hdr.vert", "hdr.frag"});

	glEnable(GL_FRAMEBUFFER_SRGB);

	GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	GLuint depthMap;
	glGenTextures(1, &depthMap);

	glTextureParameteriEXT(depthMap, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteriEXT(depthMap, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteriEXT(depthMap, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteriEXT(depthMap, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureImage2DEXT(depthMap,    GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, ShadowMapWidth, ShadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);


	glNamedFramebufferTexture2DEXT(depthMapFBO, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);

	glGetIntegerv(GL_MAX_SAMPLES, &MSAASampleCountMax);
	std::cerr << "[renderer] max MSAA samples: " << MSAASampleCountMax << '\n';

	glGenQueries(1, &m_gpu_time_query);
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_shadowmap_to);
	glDeleteFramebuffers(1, &m_shadowmap_fbo);
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
	MSAASampleCount = msaa_level ? std::exp2(msaa_level) : 0;

	MSAASampleCount = std::min(MSAASampleCount, MSAASampleCountMax);

	m_window_width = width;
	m_window_height = height;

	m_backbuffer_width  = width * m_backbuffer_scale;
	m_backbuffer_height = height * m_backbuffer_scale;

	// reinitialize multisampled color texture object
	glDeleteTextures(1, &m_backbuffer_color_to);
	glGenTextures(1,    &m_backbuffer_color_to);

	glTextureStorage2DMultisampleEXT(m_backbuffer_color_to,
	                                 GL_TEXTURE_2D_MULTISAMPLE,
	                                 MSAASampleCount,
	                                 GL_RGBA16F,
	                                 m_backbuffer_width,
	                                 m_backbuffer_height,
	                                 GL_FALSE);

	glTextureParameteriEXT(m_backbuffer_color_to, GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteriEXT(m_backbuffer_color_to, GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	// reinitialize multisampled depth texture object
	glDeleteTextures(1, &m_backbuffer_depth_to);
	glGenTextures(1,    &m_backbuffer_depth_to);

	// TODO: research if GL_DEPTH24_STENCIL8 is viable here
	glTextureStorage2DMultisampleEXT(m_backbuffer_depth_to,
	                                 GL_TEXTURE_2D_MULTISAMPLE,
	                                 MSAASampleCount,
	                                 GL_DEPTH_COMPONENT32F,
	                                 m_backbuffer_width,
	                                 m_backbuffer_height,
	                                 GL_FALSE);


	// reinitialize framebuffer
	glDeleteFramebuffers(1, &m_backbuffer_fbo);
	glGenFramebuffers(1,    &m_backbuffer_fbo);

	// attach texture objects
	glNamedFramebufferTexture2DEXT(m_backbuffer_fbo,
	                               GL_COLOR_ATTACHMENT0,
	                               GL_TEXTURE_2D_MULTISAMPLE,
	                               m_backbuffer_color_to,
	                               0);
	glNamedFramebufferTexture2DEXT(m_backbuffer_fbo,
	                               GL_DEPTH_ATTACHMENT,
	                               GL_TEXTURE_2D_MULTISAMPLE,
	                               m_backbuffer_depth_to,
	                               0);


	GLenum fboStatus = glCheckNamedFramebufferStatusEXT(m_backbuffer_fbo, GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("[renderer] could not resize backbuffer!");
	}

	m_scene->main_camera.update_projection(float(m_backbuffer_width) / (float)m_backbuffer_height);
	std::cerr << "[renderer] framebuffer resized to " << m_backbuffer_width << " x " << m_backbuffer_height << '\n';

}

static void renderQuad()
{
	static GLuint vao = 0;
	static GLuint vbo = 0;
	if (unlikely(vao == 0)) {
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 1.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
		};
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		assert(vao && vbo);
		glNamedBufferStorageEXT(vbo, sizeof(quadVertices), &quadVertices, 0);
		glEnableVertexArrayAttribEXT(vao, 0);
		glVertexArrayVertexAttribOffsetEXT(vao, vbo, 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
		glEnableVertexArrayAttribEXT(vao, 1);
		glVertexArrayVertexAttribOffsetEXT(vao, vbo, 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (3 * sizeof(float)));
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
		unif::v3(shader, indexed_uniform("point_light", ".position",  i),  m_scene->point_lights[i].position());
		unif::v3(shader, indexed_uniform("point_light", ".ambient",   i),  m_scene->point_lights[i].ambient());
		unif::v3(shader, indexed_uniform("point_light", ".diffuse",   i),  m_scene->point_lights[i].diffuse());
		unif::v3(shader, indexed_uniform("point_light", ".specular",  i),  m_scene->point_lights[i].specular());
		unif::f1(shader, indexed_uniform("point_light", ".radius",    i),  m_scene->point_lights[i].radius());
		unif::f1(shader, indexed_uniform("point_light", ".intensity", i),  m_scene->point_lights[i].intensity());
	}
}

void Renderer::draw()
{
	if(unlikely(!m_shader)) return;
	m_shader_set.check_updates();

	if(unlikely(m_scene->desired_render_scale != m_backbuffer_scale || m_scene->desired_msaa_level != msaa_level))
		resize(m_window_width, m_window_height, true);

	glBeginQuery(GL_TIME_ELAPSED, m_gpu_time_query);

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
			auto submesh_aabb = submesh.aabb;
			submesh_aabb.translate(model.transform[3]);

			int light_count = 0;
			for(unsigned i = 0; i < m_scene->point_lights.size() && i < MaxLights; ++i) {
				const auto& light = m_scene->point_lights[i];
				if(!light.enabled)
					continue;

				// TODO: implement per-light AABB cutoff to prevent light leaking
				if(submesh_aabb.intersects_sphere(light.position(), light.radius())) {
					auto dist = glm::length(light.position() - submesh_aabb.closest_point(light.position()));
					if(light.falloff(dist) > 0.0001f) {
						unif::i1(*m_shader, indexed_uniform("active_light_indices", "", light_count++), i);
					}
				}

			}
			unif::i1(*m_shader, "num_active_lights", light_count);
			material.bind(*m_shader);
			glBindVertexArray(submesh.vao);
			assert(submesh.index_type == GL_UNSIGNED_INT || submesh.index_type == GL_UNSIGNED_SHORT);
			glDrawElements(GL_TRIANGLES, submesh.numverts, submesh.index_type, nullptr);
		}
	}

	draw_skybox();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_window_width, m_window_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(*m_hdr_shader);
	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_color_to);
	unif::f1(*m_hdr_shader, "exposure", m_scene->main_camera.exposure);
	unif::i2(*m_hdr_shader, "texture_size", m_backbuffer_width, m_backbuffer_height);
	unif::i1(*m_hdr_shader, "sample_count", MSAASampleCount);
	renderQuad();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	glEndQuery(GL_TIME_ELAPSED);
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
	char avgbuf[64];
	snprintf(avgbuf, std::size(avgbuf),
	         "GPU avg: %5.2f ms (%4.1f fps), last: %5.2f ms",
	         gpu_time_avg,
	         1000.0f / std::max(gpu_time_avg, 0.01f), getTimeElapsed());

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.00f, 0.00f, 0.00f, 0.50f));
	ImGui::PlotLines("", gpu_times, std::size(gpu_times), 0, avgbuf, 0.0f, 20.0f, ImVec2(0,40));
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

float Renderer::getTimeElapsed()
{
	uint64_t time_elapsed = 0;
	glGetQueryObjectui64v(m_gpu_time_query, GL_QUERY_RESULT_NO_WAIT, &time_elapsed);
	float gpu_time = (double)time_elapsed / 1000000.0;
	gpu_time_avg = 0.0f;

	for(unsigned i = 0; i < std::size(gpu_times)-1; ++i) {
		gpu_times[i] = gpu_times[i+1];
		gpu_time_avg += gpu_times[i+1];
	}
	gpu_times[std::size(gpu_times)-1] = gpu_time;
	gpu_time_avg += gpu_time;
	gpu_time_avg /= std::size(gpu_times);

	return gpu_time;
}
