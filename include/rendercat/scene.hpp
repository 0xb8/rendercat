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

namespace rc {

struct DirectionalLight
{
	zcm::vec3 direction;
	zcm::vec3 ambient;
	zcm::vec3 diffuse;
	zcm::vec3 specular;
};

struct ExponentialDirectionalFog
{
	/// .rgb - color, .a - max opacity
	zcm::vec4 inscattering_color;
	zcm::vec4 dir_inscattering_color;

	/// amount of light scattered by fog
	float inscattering_density = 0.01f;

	/// amount of light absorbed by fog
	float extinction_density   = 0.005f;

	/// size of directional inscattering cone
	float dir_exponent = 8.0f;

	float start_distance = 0.0f;

	enum State {
		NoState,
		Enabled = 0x1,
		SynchronizeDensities = 0x2
	};
	uint8_t state = State::Enabled;
};

struct Model
{
	std::string name;
	std::unique_ptr<uint32_t[]> materials;
	std::unique_ptr<uint32_t[]> submeshes;
	uint32_t submesh_count;

	zcm::vec3 position;
	zcm::vec3 scale = 1.0f;
	zcm::quat rotation;

	zcm::mat4 transform;
	zcm::mat4 inv_transform;

	void update_transform()
	{
		rotation = zcm::normalize(rotation);
		transform = zcm::translate(zcm::scale(zcm::mat4{1.0f}, scale), position);
		transform *= zcm::mat4_cast(rotation);

		inv_transform = zcm::inverse(transform);
	}
};

struct Scene
{
	Scene() = default;

	static constexpr size_t missing_material_idx = 0u;

	std::vector<rc::Material>    materials;
	std::vector<model::Mesh> submeshes;
	std::vector<Model>       models;
	std::vector<PointLight>  point_lights;
	std::vector<SpotLight>   spot_lights;

	bool  window_shown = true;

	void init();
	void update();

	rc::Camera main_camera;

	DirectionalLight directional_light
	{
		zcm::vec3(0.217f, 0.468f, 0.857f), // dir
		zcm::vec3(0.029f, 0.034f, 0.081f), // amb
		zcm::vec3(0.596f, 0.364f, 0.187f), // diff
		zcm::vec3(0.554f, 0.332f, 0.106f)  // spec
	};

	ExponentialDirectionalFog fog
	{
		zcm::vec4(0.029f, 0.034f, 0.081f, 1.0f), // ins
		zcm::vec4(0.109f, 0.071f, 0.042f, 1.0f)  // dir ins
	};

	Cubemap cubemap;
};

}
