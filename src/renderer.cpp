#include <renderer.hpp>
#include <scene.hpp>
#include <iostream>
#include <cstdio>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static glm::mat4 make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear)
{
	float f = 1.0f / tan(fovY_radians / 2.0f);
	return glm::mat4(f / aspectWbyH, 0.0f,  0.0f,  0.0f,
	                 0.0f,    f,  0.0f,  0.0f,
	                 0.0f, 0.0f,  0.0f, -1.0f,
	                 0.0f, 0.0f, zNear,  0.0f);
}
#if 0
static glm::mat4 make_default_projection(float fov_rad, float aspect, float znear, float zfar)
{
	return glm::perspective(fov_rad, aspect, znear, zfar);
}
#endif

void Renderer::update_projection()
{
	float aspect = (float)m_backbuffer_width / (float)m_backbuffer_height;
	if(m_last_fov != m_scene->main_camera.fov || m_last_aspect != aspect)
	{
		m_last_fov =  m_scene->main_camera.fov;
		m_last_aspect = aspect;
		m_projection = make_reversed_z_projection(glm::radians(m_scene->main_camera.fov),
		                                         aspect,
		                                         m_scene->main_camera.znear);
	}
}

Renderer::Renderer(Scene * s) : m_scene(s)
{
	assert(s != nullptr);

	m_shader = m_shader_set.load_program({"generic.vert", "generic.frag"});
	m_shadowmap_shader = m_shader_set.load_program({"shadow_mapping.vert", "shadow_mapping.frag"});
	m_debug_quad_shader = m_shader_set.load_program({"debug_quad.vert", "debug_quad.frag"});

	glEnable(GL_FRAMEBUFFER_SRGB);

	GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, ShadowMapWidth, ShadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_shadowmap_fbo = depthMapFBO;
	m_shadowmap_to = depthMap;
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_shadowmap_to);
	glDeleteFramebuffers(1, &m_shadowmap_fbo);
	glDeleteTextures(1, &m_backbuffer_color_to);
	glDeleteTextures(1, &m_backbuffer_depth_to);
	glDeleteFramebuffers(1, &m_backbuffer_fbo);
}

void Renderer::resize(int width, int height)
{
	m_backbuffer_width = width;
	m_backbuffer_height = height;

	// reinitialize multisampled color texture object
	glDeleteTextures(1, &m_backbuffer_color_to);
	glGenTextures(1,    &m_backbuffer_color_to);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_color_to);
	// TODO: research why SRGB conversion breaks when using RGBA16F format
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MSAASampleCount, GL_SRGB8_ALPHA8, m_backbuffer_width, m_backbuffer_height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	// reinitialize multisampled depth texture object
	glDeleteTextures(1, &m_backbuffer_depth_to);
	glGenTextures(1,    &m_backbuffer_depth_to);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_depth_to);
	// TODO: research if GL_DEPTH24_STENCIL8 is viable here
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MSAASampleCount, GL_DEPTH_COMPONENT32F, m_backbuffer_width, m_backbuffer_height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	// reinitialize framebuffer
	glDeleteFramebuffers(1, &m_backbuffer_fbo);
	glGenFramebuffers(1,    &m_backbuffer_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_backbuffer_fbo);

	// attach texture objects
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_color_to, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_depth_to,  0);
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("[renderer] could not resize backbuffer!");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	update_projection();
	std::cerr << "[renderer] framebuffer resized to " << m_backbuffer_width << " x " << m_backbuffer_height << '\n';

}

static void renderQuad()
{
	static unsigned int quadVAO = 0;
	static unsigned int quadVBO;
	if (quadVAO == 0)
	{
		float quadVertices[] = {
		        // positions        // texture Coords
		        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}


static const char* indexed_uniform(std::string_view array, std::string_view name, unsigned idx)
{
	static char buf[128] = {0};
	snprintf(buf, std::size(buf), "%s[%d].%s", array.data(), idx, name.data());
	return &buf[0];
}

void Renderer::draw()
{
	if(!m_shader) return;
	m_shader_set.check_updates();

	glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
	glClearDepth(0.0f);
	glDepthFunc(GL_GREATER);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

#if 0
	glDisable(GL_CULL_FACE);

	glUseProgram(*m_shadowmap_shader);

	glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;
        float near_plane = 15.0f, far_plane = 30.0f;
        lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane);
        lightView = glm::lookAt(-m_scene->directional_light.direction, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

	unif::m4(*m_shadowmap_shader, "lightSpace", lightSpaceMatrix);

	glViewport(0, 0, ShadowMapWidth, ShadowMapHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowmap_fbo);
	glClear(GL_DEPTH_BUFFER_BIT);

	for(const auto& instance : m_scene->instances) {
		m_scene->materials[instance.material_id].bind(*m_shadowmap_shader);
		glm::mat4 model;
		model = glm::translate(model, instance.position);
		model = glm::rotate(model, instance.angle, instance.axis);
		model = glm::scale(model, instance.scale);
		unif::m4(*m_shadowmap_shader, "model", model);

		glBindVertexArray(m_scene->meshes[instance.mesh_id].vao);
		glDrawElements(GL_TRIANGLES, m_scene->meshes[instance.mesh_id].numverts, GL_UNSIGNED_INT, nullptr);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#endif

	// set out framebuffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, m_backbuffer_fbo);
	glViewport(0, 0, m_backbuffer_width, m_backbuffer_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);





	glUseProgram(*m_shader);

	update_projection();

	// camera/view transformation
	glm::mat4 view = glm::lookAt(m_scene->main_camera.pos,
	                             m_scene->main_camera.pos + m_scene->main_camera.front,
	                             m_scene->main_camera.up);

	unif::m4(*m_shader, "projection", m_projection);
	unif::m4(*m_shader, "view", view);
//	unif::m4(*m_shader, "lightSpace", lightSpaceMatrix);
	unif::v3(*m_shader, "viewPos", m_scene->main_camera.pos);
	unif::v3(*m_shader, "directional_light.direction", m_scene->directional_light.direction);
	unif::v3(*m_shader, "directional_light.ambient",   m_scene->directional_light.ambient);
	unif::v3(*m_shader, "directional_light.diffuse",   m_scene->directional_light.diffuse);
	unif::v3(*m_shader, "directional_light.specular",  m_scene->directional_light.specular);
	unif::i1(*m_shader, "num_active_lights", (int)m_scene->lights.size());

	for(unsigned i = 0; i < m_scene->lights.size() && i < 8; ++i) {
		unif::v3(*m_shader, indexed_uniform("point_light", "position",  i),  m_scene->lights[i].position());
		unif::v3(*m_shader, indexed_uniform("point_light", "ambient",   i),  m_scene->lights[i].ambient());
		unif::v3(*m_shader, indexed_uniform("point_light", "diffuse",   i),  m_scene->lights[i].diffuse());
		unif::v3(*m_shader, indexed_uniform("point_light", "specular",  i),  m_scene->lights[i].specular());
		unif::f1(*m_shader, indexed_uniform("point_light", "radius",    i),  m_scene->lights[i].radius());
		unif::f1(*m_shader, indexed_uniform("point_light", "intensity", i),  m_scene->lights[i].intensity());
	}

//	glActiveTexture(GL_TEXTURE2);
//	glBindTexture(GL_TEXTURE_2D, m_shadowmap_to);
//	unif::i1(*m_shader, "shadowMap", 2);

	for(const auto& instance : m_scene->instances) {
		m_scene->materials[instance.material_id].bind(*m_shader);

		glm::mat4 model;
		model = glm::translate(model, instance.position);
		model = glm::rotate(model, instance.angle, instance.axis);
		model = glm::scale(model, instance.scale);
		unif::m4(*m_shader, "model", model);
		unif::m3(*m_shader, "normal_matrix", glm::transpose(glm::inverse(glm::mat3(model))));

		glBindVertexArray(m_scene->meshes[instance.mesh_id].vao);
		glDrawElements(GL_TRIANGLES, m_scene->meshes[instance.mesh_id].numverts, GL_UNSIGNED_INT, nullptr);
	}


	glUseProgram(0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	// resolve multisampled framebuffer into default backbuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 /* default fbo */);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_backbuffer_fbo);
	//glDrawBuffer(GL_BACK); // dunno if needed
	glBlitFramebuffer(0, 0, m_backbuffer_width, m_backbuffer_height,
	                  0, 0, m_backbuffer_width, m_backbuffer_height,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}
