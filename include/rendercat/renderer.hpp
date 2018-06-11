#pragma once

#include <rendercat/shader_set.hpp>
#include <rendercat/util/gl_perfquery.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <debug_draw.hpp>
#include <vector>

namespace rc {

class Scene;

class Renderer
{
	ShaderSet m_shader_set;
	PerfQuery m_perfquery;
	Scene*  m_scene;

	uint32_t* m_shader = nullptr;
	uint32_t* m_cubemap_shader = nullptr;
	uint32_t* m_hdr_shader = nullptr;
	uint32_t* m_shadow_shader = nullptr;

	uint32_t m_backbuffer_width  = 0;
	uint32_t m_backbuffer_height = 0;
	uint32_t m_window_width  = 0;
	uint32_t m_window_height = 0;
	float    m_backbuffer_scale  = 1.0f;

	rc::framebuffer_handle m_shadowmap_fbo;
	rc::texture_handle     m_shadowmap_depth_to;

	rc::framebuffer_handle m_backbuffer_fbo;
	rc::texture_handle     m_backbuffer_color_to;
	rc::texture_handle     m_backbuffer_depth_to;

	rc::framebuffer_handle m_backbuffer_resolve_fbo;
	rc::texture_handle     m_backbuffer_resolve_color_to;

	void set_uniforms(uint32_t shader);
	glm::mat4 set_shadow_uniforms();
	void draw_shadow();
	void draw_skybox();
	void init_shadow();

	DDRenderInterfaceCoreGL debug_draw_ctx;

	struct ModelMeshIdx
	{
		uint32_t model_idx;
		uint32_t submesh_idx;
	};
	std::vector<ModelMeshIdx> m_masked_meshes;
	std::vector<ModelMeshIdx> m_blended_meshes;

public:
	static const unsigned int ShadowMapWidth = 2048;
	static const unsigned int ShadowMapHeight = 2048;

	// BUG: when MSAA sample count is >0, specular mapping induces random white dots on some meshes.
	// This is partially mitigated in shader, but still present when fragment is lit by several light sources.
	int msaa_level = 0;
	int MSAASampleCount = 1;
	int MSAASampleCountMax = 4;
	static constexpr int MaxLights = 16;

	explicit Renderer(Scene* s);
	~Renderer() = default;

	void resize(uint32_t width, uint32_t height, bool force = false);
	void draw();
	void draw_gui();
};

} // namespace rc
