#pragma once

#include <rendercat/shader_set.hpp>
#include <rendercat/util/gl_perfquery.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <rendercat/uniform.hpp>
#include <debug_draw_interface.hpp>
#include <zcm/mat3.hpp>
#include <zcm/mat4.hpp>
#include <rendercat/core/bbox.hpp>
#include <vector>

namespace rc {

struct Scene;

class Renderer
{
	ShaderSet m_shader_set;
	PerfQuery m_perfquery;
	Scene*  m_scene;

	uint32_t* m_shader = nullptr;
	uint32_t* m_hdr_shader = nullptr;
	uint32_t* m_shadow_shader = nullptr;
	uint32_t* m_shadow_masked_shader = nullptr;
	uint32_t* m_brdf_shader = nullptr;

	uint32_t m_backbuffer_width  = 0;
	uint32_t m_backbuffer_height = 0;
	uint32_t m_window_width  = 0;
	uint32_t m_window_height = 0;
	float    m_backbuffer_scale  = 1.0f;
	float    m_device_pixel_ratio = 1.0f;

	rc::texture_handle     m_brdf_lut_to;

	rc::framebuffer_handle m_shadowmap_fbo;
	rc::texture_handle     m_shadowmap_depth_to;

	rc::framebuffer_handle m_spot_shadow_fbo;
	rc::texture_handle     m_spot_shadow_depth_to;

	rc::framebuffer_handle m_backbuffer_fbo;
	rc::texture_handle     m_backbuffer_color_to;
	rc::texture_handle     m_backbuffer_depth_to;

	rc::framebuffer_handle m_backbuffer_resolve_fbo;
	rc::texture_handle     m_backbuffer_resolve_color_to;

	void set_uniforms();
	void set_shadow_uniforms();

	void draw_shadow();
	void draw_spot_shadow();
	void draw_skybox();
	void init_shadow();
	void init_brdf();

	DDRenderInterfaceCoreGL debug_draw_ctx;

	struct MeshTransform {
		zcm::mat4 mat;
		zcm::mat4 inv_mat;
		bbox3 transformed_bbox;
	};

	std::vector<MeshTransform> m_transform_cache;

	struct ModelMeshIdx
	{
		uint32_t model_idx;
		uint32_t submesh_idx;
		uint32_t transform_idx;
	};
	std::vector<ModelMeshIdx> m_opaque_meshes;
	std::vector<ModelMeshIdx> m_masked_meshes;
	std::vector<ModelMeshIdx> m_blended_meshes;

	static constexpr size_t RC_MAX_LIGHTS = 16;

	struct alignas(256) PerFrameData {
		zcm::mat4 proj_view;
		zcm::mat4 light_proj_view;
		zcm::vec3 camera_forward;
		float     znear;
		zcm::vec3 viewPos;
		float     _pading1;

		struct DirectionalLight {
			zcm::vec4 color_intensity;
			zcm::vec3 direction;
			float _padding1;
		};
		DirectionalLight dir_light[1];

		struct DirectionalFog {
			zcm::vec4 inscattering_color;
			zcm::vec4 dir_inscattering_color;
			zcm::vec4 direction_exponent;
			float inscattering_density;
			float extinction_density;
			bool enabled;
			uint32_t _padding1;
		};
		DirectionalFog dir_fog;


		struct PointLight {
			zcm::vec4 position_radius; // .xyz - pos,   .w - radius
			zcm::vec4 color_intensity; // .rgb - color, .a - luminous intensity (candela)
		};
		PointLight point_lights[RC_MAX_LIGHTS];

		struct SpotLight : public PointLight {
			zcm::vec4 direction_angle_scale; // .xyz - dir,   .w - angle scale
			zcm::vec4 angle_offset;
		};
		SpotLight spot_lights[RC_MAX_LIGHTS];

		int32_t num_msaa_samples;
		bool shadows_enabled;
	};

	struct alignas(256) LightPerframeData {
		zcm::mat4 spot_light_matrices[RC_MAX_LIGHTS];

		// std140 array alignment
		struct alignas(16) LightIndex {
			int index;
		};

		LightIndex shadow_indices[RC_MAX_LIGHTS];
	};

	unif::buf<PerFrameData, 2> m_per_frame;
	unif::buf<LightPerframeData, 2> m_light_per_frame;

	zcm::mat4 m_shadow_matrix;

public:
	static const unsigned int PointShadowWidth = 512;
	static const unsigned int PointShadowHeight = 512;

	static const unsigned int ShadowMapWidth = 2048;
	static const unsigned int ShadowMapHeight = 2048;

	// BUG: when MSAA sample count is >0, specular mapping induces random white dots on some meshes.
	// This is partially mitigated in shader, but still present when fragment is lit by several light sources.
	int msaa_level = 0;
	int MSAASampleCount = 1;
	int MSAASampleCountMax = 4;
	int desired_msaa_level = 2;
	int selected_cubemap = 0;
	int cubemap_mip_level = 0;

	float desired_render_scale = 1.0f;
	bool draw_mesh_bboxes = false;
	bool draw_model_bboxes = false;
	bool do_shadow_mapping = true;
	bool window_shown = true;

	static constexpr int MaxLights = RC_MAX_LIGHTS;

	explicit Renderer(Scene* s);
	~Renderer() = default;

	void resize(uint32_t width, uint32_t height, float device_pixel_ratio, bool force = false);
	void clear_screen();
	void draw();
	void draw_gui();
	void save_hdr_backbuffer(std::string_view path);
};

} // namespace rc
