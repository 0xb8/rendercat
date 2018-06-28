#pragma once

#include <rendercat/common.hpp>
#include <rendercat/mesh.hpp>
#include <rendercat/core/camera.hpp>
#include <rendercat/material.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/core/point_light.hpp>
#include <rendercat/cubemap.hpp>

#include <memory>
#include <string_view>
#include <glm/gtx/euler_angles.hpp>

namespace rc {

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct ExponentialDirectionalFog
{
	/// .rgb - color, .a - max opacity
	glm::vec4 inscattering_color;
	glm::vec4 dir_inscattering_color;

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

	glm::vec3 position;
	glm::vec3 rotation_euler;

	glm::mat4 transform;
	glm::mat4 inv_transform;

	void update_transform()
	{
		transform = glm::translate(glm::mat4(), position);
		transform *= glm::yawPitchRoll(rotation_euler.x, rotation_euler.y, rotation_euler.z);

		inv_transform = glm::inverse(transform);
	}
};

struct Scene
{
	Scene();

	static constexpr size_t missing_material_idx = 0u;
	rc::TextureCache m_texture_cache;

	std::vector<rc::Material>    materials;
	std::vector<model::Mesh> submeshes;
	std::vector<Model>       models;
	std::vector<PointLight>  point_lights;
	std::vector<SpotLight>   spot_lights;

	bool  window_shown = true;
	bool  draw_bbox = false;
	int   desired_msaa_level = 0;
	float desired_render_scale = 1.0f;

	bool shadows = true;

	void update();

	Model* load_model(const std::string_view name, const std::string_view basedir);

	rc::Camera main_camera;

	DirectionalLight directional_light
	{
		glm::vec3(0.217f, 0.468f, 0.857f), // dir
		glm::vec3(0.029f, 0.034f, 0.081f), // amb
		glm::vec3(0.596f, 0.364f, 0.187f), // diff
		glm::vec3(0.554f, 0.332f, 0.106f)  // spec
	};

	ExponentialDirectionalFog fog
	{
		glm::vec4(0.029f, 0.034f, 0.081f, 1.0f), // ins
		glm::vec4(0.109f, 0.071f, 0.042f, 1.0f)  // dir ins
	};

	Cubemap cubemap;
};

}
