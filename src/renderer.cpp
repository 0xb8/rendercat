#include <renderer.hpp>

#include <cstdio>

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

static glm::mat4 make_default_projection(float fov_rad, float aspect, float znear, float zfar)
{
	return glm::perspective(fov_rad, aspect, znear, zfar);
}


void Renderer::update_projection()
{
	if(last_fov != m_scene->main_camera.fov)
	{
		last_fov =  m_scene->main_camera.fov;
		projection = make_reversed_z_projection(glm::radians(m_scene->main_camera.fov),
		                                        (float)m_backbuffer_width / (float)m_backbuffer_height,
		                                        m_scene->main_camera.znear);
	}
}

Renderer::Renderer(Scene * s) : m_scene(s)
{
	m_shader = m_shader_set.load_program({"generic.vert", "generic.frag"});
	m_point_lamp_shader = m_shader_set.load_program({"point_lamp.vert", "point_lamp.frag"});


	glEnable(GL_FRAMEBUFFER_SRGB);
}

Renderer::~Renderer()
{

}

void Renderer::resize(int width, int height)
{
	m_backbuffer_width = width;
	m_backbuffer_height = height;

	// reinitialize multisampled color texture object
	glDeleteTextures(1, &m_backbuffer_color_to);
	glGenTextures(1,    &m_backbuffer_color_to);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_color_to);
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, GL_SRGB8_ALPHA8, m_backbuffer_width, m_backbuffer_height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	// reinitialize multisampled depth texture object
	glDeleteTextures(1, &m_backbuffer_depth_to);
	glGenTextures(1,    &m_backbuffer_depth_to);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_depth_to);
	// TODO: research if GL_DEPTH24_STENCIL8 is viable here
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, GL_DEPTH_COMPONENT32F, m_backbuffer_width, m_backbuffer_height, GL_TRUE);
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
		fprintf(stderr, "glCheckFramebufferStatus: %x\n", fboStatus);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	update_projection();
}

void Renderer::draw(float dt)
{
	if(!m_shader) return;
	m_shader_set.check_updates();

	// set out framebuffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, m_backbuffer_fbo);
	glViewport(0, 0, m_backbuffer_width, m_backbuffer_height);

	glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_GREATER);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(*m_shader);

	if(m_scene->box.material.diffuse) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_scene->box.material.diffuse);
		unif::i1(*m_shader, "material.diffuse", 0);
	}
	if(m_scene->box.material.normal) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_scene->box.material.normal);
		unif::i1(*m_shader, "material.normal", 1);
	}


	update_projection();

	// camera/view transformation
	glm::mat4 view = glm::lookAt(m_scene->main_camera.pos,
	                             m_scene->main_camera.pos + m_scene->main_camera.front,
	                             m_scene->main_camera.up);

	unif::m4(*m_shader, "projection", projection);
	unif::m4(*m_shader, "view", view);
	unif::v3(*m_shader, "viewPos", m_scene->main_camera.pos);
	unif::v3(*m_shader, "directional_light.direction", m_scene->directional_light.direction);
	unif::v3(*m_shader, "directional_light.ambient",   m_scene->directional_light.ambient);
	unif::v3(*m_shader, "directional_light.diffuse",   m_scene->directional_light.diffuse);
	unif::v3(*m_shader, "directional_light.specular",  m_scene->directional_light.specular);

	unif::v3(*m_shader, "point_light.position",  m_scene->point_light.position);
	unif::v3(*m_shader, "point_light.ambient",   m_scene->point_light.ambient);
	unif::v3(*m_shader, "point_light.diffuse",   m_scene->point_light.diffuse);
	unif::v3(*m_shader, "point_light.specular",  m_scene->point_light.specular);
	unif::f1(*m_shader, "point_light.constant",  m_scene->point_light.constant);
	unif::f1(*m_shader, "point_light.linear",    m_scene->point_light.linear);
	unif::f1(*m_shader, "point_light.quadratic", m_scene->point_light.quadratic);

	unif::v3(*m_shader,  "material.specular",  m_scene->box.material.specular);
	unif::f1(*m_shader,  "material.shininess", m_scene->box.material.shininess);


	for (unsigned int i = 0; i < 10; i++) {
		glm::mat4 model;
		model = glm::translate(model, m_scene->cubePositions[i]);
		float angle = 20.0f *i;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		unif::m4(*m_shader, "model", model);

		glBindVertexArray(m_scene->box.vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// draw light

	glUseProgram(*m_point_lamp_shader);



	glm::mat4 model;
	model = glm::translate(model, m_scene->point_light.position);
	model = glm::scale(model, glm::vec3(0.2f));

	unif::m4(*m_point_lamp_shader, "projection", projection);
	unif::m4(*m_point_lamp_shader, "view", view);
	unif::m4(*m_point_lamp_shader, "model", model);
	unif::v3(*m_point_lamp_shader, "point_light.diffuse", m_scene->point_light.diffuse);
	glBindVertexArray(m_scene->point_light.vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glUseProgram(0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	// resolve framebuffer into default backbuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // default FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_backbuffer_fbo);
	glDrawBuffer(GL_BACK); // dunno if needed
	glBlitFramebuffer(0, 0, m_backbuffer_width, m_backbuffer_height,
	                  0, 0, m_backbuffer_width, m_backbuffer_height,
	                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
}
