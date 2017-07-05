#include <renderer.hpp>

#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static const int SampleCount = 4;


Renderer::Renderer(Scene * s) : m_scene(s)
{
	m_shader = m_shader_set.load_program({"triangle.vert", "triangle.frag"});

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB);

	float aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);

	fprintf(stderr, "Max aniso: %f\n", aniso);


}

Renderer::~Renderer()
{

}

void Renderer::resize(int width, int height)
{
	m_backbuffer_width = width;
	m_backbuffer_height = height;


	glDeleteTextures(1, &m_backbuffer_color_to);
	glGenTextures(1,    &m_backbuffer_color_to);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_color_to);
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, GL_SRGB8_ALPHA8, m_backbuffer_width, m_backbuffer_height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	glDeleteTextures(1, &m_backbuffer_depth_to);
	glGenTextures(1,    &m_backbuffer_depth_to);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_backbuffer_depth_to);
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, SampleCount, GL_DEPTH_COMPONENT32F, m_backbuffer_width, m_backbuffer_height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	glDeleteFramebuffers(1, &m_backbuffer_fbo);
	glGenFramebuffers(1,    &m_backbuffer_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_backbuffer_fbo);
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

	glBindFramebuffer(GL_FRAMEBUFFER, m_backbuffer_fbo);
	glViewport(0, 0, m_backbuffer_width, m_backbuffer_height);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glUseProgram(*m_shader);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_scene->texture);

	glm::mat4 projection = glm::perspective(glm::radians(m_scene->fov), (float)m_backbuffer_width / (float)m_backbuffer_height, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(*m_shader, "projection"), 1, GL_FALSE, &projection[0][0]);

	// camera/view transformation
	glm::mat4 view = glm::lookAt(m_scene->cameraPos, m_scene->cameraPos + m_scene->cameraFront, m_scene->cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(*m_shader, "view"), 1, GL_FALSE, &view[0][0]);

	glm::mat4 model;
	float angle = 20.0f;
	model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f * cos(dt), 0.3f * sin(dt), 0.5f));
	glUniformMatrix4fv(glGetUniformLocation(*m_shader, "model"), 1, GL_FALSE, &model[0][0]);

	glBindVertexArray(m_scene->vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);



	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // default FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_backbuffer_fbo);
	glDrawBuffer(GL_BACK);
	glBlitFramebuffer(
	0, 0, m_backbuffer_width, m_backbuffer_height,
	0, 0, m_backbuffer_width, m_backbuffer_height,
	GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
