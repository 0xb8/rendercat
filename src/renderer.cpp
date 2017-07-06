#include <renderer.hpp>

#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static const int SampleCount = 0;

static glm::mat4 MakeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float zNear)
{
   float f = 1.0f / tan(fovY_radians / 2.0f);
   return glm::mat4(
       f / aspectWbyH, 0.0f,  0.0f,  0.0f,
		 0.0f,    f,  0.0f,  0.0f,
		 0.0f, 0.0f,  0.0f, -1.0f,
		 0.0f, 0.0f, zNear,  0.0f);
}


Renderer::Renderer(Scene * s) : m_scene(s)
{
	m_shader = m_shader_set.load_program({"generic.vert", "generic.frag"});

	glEnable(GL_DEPTH_TEST);
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
}

void Renderer::draw(float dt)
{
	if(!m_shader) return;

	// set out framebuffer and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, m_backbuffer_fbo);
	glViewport(0, 0, m_backbuffer_width, m_backbuffer_height);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_GREATER);
	glEnable(GL_DEPTH_TEST);



	glUseProgram(*m_shader);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_scene->texture);

//	glm::mat4 projection = glm::perspective(
//	                               glm::radians(m_scene->main_camera.fov),
//	                               (float)m_backbuffer_width / (float)m_backbuffer_height,
//	                               m_scene->main_camera.znear,
//	                               m_scene->main_camera.zfar);


	glm::mat4 projection = MakeInfReversedZProjRH(glm::radians(m_scene->main_camera.fov),
	                                              (float)m_backbuffer_width / (float)m_backbuffer_height,
	                                              m_scene->main_camera.znear);


	glUniformMatrix4fv(glGetUniformLocation(*m_shader, "projection"), 1, GL_FALSE, &projection[0][0]);

	// camera/view transformation
	glm::mat4 view = glm::lookAt(m_scene->main_camera.pos,
	                             m_scene->main_camera.pos + m_scene->main_camera.front,
	                             m_scene->main_camera.up);
	glUniformMatrix4fv(glGetUniformLocation(*m_shader, "view"), 1, GL_FALSE, &view[0][0]);

	for (unsigned int i = 0; i < 10; i++) {

		glm::mat4 model;
		model = glm::translate(model, m_scene->cubePositions[i]);
		float angle = 20.0f *i;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f * cos(dt), 0.3f * sin(dt), 0.5f));
		glUniformMatrix4fv(glGetUniformLocation(*m_shader, "model"), 1, GL_FALSE, &model[0][0]);

		glBindVertexArray(m_scene->vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	// resolve framebuffer into default backbuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // default FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_backbuffer_fbo);
	glDrawBuffer(GL_BACK); // dunno if needed
	glBlitFramebuffer(0, 0, m_backbuffer_width, m_backbuffer_height,
	                  0, 0, m_backbuffer_width, m_backbuffer_height,
	                  GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
}
