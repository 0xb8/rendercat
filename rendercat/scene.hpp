#pragma once

#include <zcm/vec3.hpp>
#include <zcm/vec4.hpp>
#include <zcm/mat4.hpp>
#include <zcm/matrix.hpp>
#include <zcm/matrix_transform.hpp>
#include <zcm/quaternion.hpp>
#include <zcm/common.hpp>

#include <rendercat/common.hpp>
#include <rendercat/mesh.hpp>
#include <rendercat/core/camera.hpp>
#include <rendercat/material.hpp>
#include <rendercat/core/point_light.hpp>
#include <rendercat/cubemap.hpp>

#include <memory>
#include <string_view>
#include <filesystem>

namespace rc {

struct DirectionalLight
{
	/// .rgb - color, .a - intensity
	zcm::vec4 color_intensity;

	zcm::quat direction;

	float ambient_intensity;
};

struct ExponentialDirectionalFog
{
	/// .rgb - color, .a - max opacity
	zcm::vec4 dir_inscattering_color;

	/// environment (cubemap) fog opacity factor.
	float inscattering_environment_opacity = 1.0f;

	/// amount of light scattered by fog
	float inscattering_density = 0.01f;

	/// amount of light absorbed by fog
	float extinction_density   = 0.005f;

	/// size of directional inscattering cone
	float dir_exponent = 16.0f;

	enum State {
		NoState,
		Enabled = 0x1,
		SynchronizeDensities = 0x2
	};
	uint8_t state = State::Enabled;
};

struct RigidTransform {

	zcm::vec3 position;
	zcm::vec3 scale = 1.0f;
	zcm::quat rotation;

	zcm::mat4 mat;
	zcm::mat4 inv_mat;

	zcm::mat4 get_mat() const
	{
		// TODO: optimize
		auto scale_mat = zcm::scale(zcm::mat4{1}, scale);
		auto rotate_mat = zcm::mat4_cast(rotation);
		auto translate_mat = zcm::translate(zcm::mat4{1}, position);
		return translate_mat * rotate_mat * scale_mat;
	}

	zcm::mat4 get_inv_mat() const
	{
		// calculate separate inverse matrices (faster and more precice than inverse())
		auto scale_mat_inv = zcm::scale(zcm::mat4{1}, 1.0f / scale);
		auto rotate_mat_inv = zcm::mat4_cast(zcm::conjugate(rotation));
		auto translate_mat_inv = zcm::translate(zcm::mat4{1}, -position);
		// (AB)^-1 = B^-1 * A^-1
		return scale_mat_inv * rotate_mat_inv * translate_mat_inv;
	}

	void update()
	{
		rotation = zcm::normalize(rotation);
		mat = get_mat();
		inv_mat = get_inv_mat();
	}

};


struct ShadedMesh
{
	uint32_t mesh;
	uint32_t material;
	RigidTransform transform;
};


struct Model
{
	std::string name;
	std::unique_ptr<uint32_t[]> shaded_meshes;
	uint32_t mesh_count;
	RigidTransform transform;
	rc::bbox3 bbox;
};


struct Scene
{
	Scene() = default;
	~Scene();

	static constexpr size_t missing_material_idx = 0u;

	std::vector<rc::Material> materials;
	std::vector<model::Mesh>  submeshes;
	std::vector<ShadedMesh>   shaded_meshes;
	std::vector<Model>        models;

	std::vector<PointLight>   point_lights;
	std::vector<SpotLight>    spot_lights;

	bool  window_shown = true;

	void init();
	void update();

	void load_skybox_equirectangular(std::string_view path);
	void load_skybox_cubemap(std::string_view path);
	void skyboxes_list();

	Model* load_model_gltf(const std::filesystem::path& file);

	rc::Camera main_camera;

	DirectionalLight directional_light;
	ExponentialDirectionalFog fog;

	std::string current_cubemap;
	Cubemap cubemap;
	Cubemap cubemap_diffuse_irradiance;
	Cubemap cubemap_specular_environment;
};

}
