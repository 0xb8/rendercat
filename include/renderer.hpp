#pragma once

#include <scene.hpp>
#include <shader_set.hpp>

class Scene;

class Renderer
{

	Scene* m_scene;
	ShaderSet m_shader_set;
	GLuint* m_shader;
	GLuint* m_point_lamp_shader;

	int m_backbuffer_width  = 0;
	int m_backbuffer_height = 0;

	GLuint m_backbuffer_fbo = 0;
	GLuint m_backbuffer_color_to = 0;
	GLuint m_backbuffer_depth_to = 0;

	glm::mat4 projection;
	float last_fov = 0.0f;

	glm::vec4 clear_color{0.0f, 0.0f, 0.0f, 1.0f};


	void update_projection();



public:
	static constexpr int SampleCount = 0;

	explicit Renderer(Scene* s = nullptr);
	~Renderer();

	void resize(int width, int height);

	void draw(float dt);
};
