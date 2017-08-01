#pragma once

#include <shader_set.hpp>

class Scene;

class Renderer
{

	ShaderSet m_shader_set;
	Scene*  m_scene;
	GLuint* m_shader = nullptr;
	GLuint* m_shadowmap_shader = nullptr;
	GLuint* m_cubemap_shader = nullptr;

	int m_backbuffer_width  = 0;
	int m_backbuffer_height = 0;

	GLuint m_backbuffer_fbo = 0;
	GLuint m_shadowmap_fbo = 0;
	GLuint m_backbuffer_color_to = 0;
	GLuint m_backbuffer_depth_to = 0;
	GLuint m_shadowmap_to = 0;

	glm::mat4 m_projection;
	glm::vec4 m_clear_color {0.0f, 0.0f, 0.0f, 1.0f};
	float m_last_fov = 0.0f;
	float m_last_aspect = 0.0f;

	void update_projection();

	void draw_skybox();

public:
	static const unsigned int ShadowMapWidth = 1024;
	static const unsigned int ShadowMapHeight = 1024;
	static constexpr int MSAASampleCount = 0;

	explicit Renderer(Scene* s);
	~Renderer();

	void resize(int width, int height);
	void draw();
};
