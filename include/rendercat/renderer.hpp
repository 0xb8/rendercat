#pragma once

#include <rendercat/shader_set.hpp>
#include <rendercat/gl_perfquery.hpp>

class Scene;

class Renderer
{

	ShaderSet m_shader_set;
	PerfQuery m_perfquery;
	Scene*  m_scene;

	gl::GLuint* m_shader = nullptr;
	gl::GLuint* m_cubemap_shader = nullptr;
	gl::GLuint* m_hdr_shader = nullptr;

	uint32_t m_backbuffer_width  = 0;
	uint32_t m_backbuffer_height = 0;
	uint32_t m_window_width  = 0;
	uint32_t m_window_height = 0;
	float    m_backbuffer_scale  = 1.0f;

	gl::GLuint m_backbuffer_fbo = 0;
	gl::GLuint m_backbuffer_color_to = 0;
	gl::GLuint m_backbuffer_depth_to = 0;

	void set_uniforms(gl::GLuint shader);
	void draw_skybox();

public:
	static const unsigned int ShadowMapWidth = 1024;
	static const unsigned int ShadowMapHeight = 1024;

	// BUG: when MSAA sample count is >0, specular mapping induces random white dots on some meshes.
	// This is partially mitigated in shader, but still present when fragment is lit by several light sources.
	int msaa_level = 0;
	int MSAASampleCount = 1;
	int MSAASampleCountMax = 4;
	static constexpr int MaxLights = 16;

	explicit Renderer(Scene* s);
	~Renderer();

	void resize(uint32_t width, uint32_t height, bool force = false);
	void draw();
	void draw_gui();
};
