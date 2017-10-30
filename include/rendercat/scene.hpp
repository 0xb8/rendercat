#pragma once

#include <rendercat/common.hpp>
#include <rendercat/mesh.hpp>
#include <rendercat/camera.hpp>
#include <rendercat/material.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/point_light.hpp>
#include <rendercat/cubemap.hpp>

#include <memory>
#include <string_view>
#include <glm/gtx/euler_angles.hpp>

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
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
	TextureCache m_texture_cache;

	std::vector<Material>    materials;
	std::vector<model::mesh> submeshes;
	std::vector<Model>       models;
	std::vector<PointLight>  point_lights;
	std::vector<SpotLight>   spot_lights;
	size_t flashlight_idx =  std::numeric_limits<size_t>::max();

	bool  window_shown = true;
	int   desired_msaa_level = 0;
	float desired_render_scale = 1.0f;

	void update();

	Model* load_model(const std::string_view name, const std::string_view basedir);

	Camera main_camera;

	DirectionalLight directional_light {
		glm::vec3(0.3f, 1.0f, 0.8f),       // dir
		glm::vec3(0.029f, 0.034f, 0.081f), // amb
		glm::vec3(0.109f, 0.071f, 0.042f), // diff
		glm::vec3(0.032f, 0.021f, 0.012f)};// spec

	Cubemap cubemap;
};
