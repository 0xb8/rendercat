#pragma once

#include <common.hpp>
#include <string_view>
#include <map>
#include <mesh.hpp>
#include <camera.hpp>
#include <material.hpp>
#include <point_light.hpp>

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};



struct Instance
{
	uint32_t material_id;
	uint32_t mesh_id;

	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 scale = {1.0f, 1.0f, 1.0f};
	glm::vec3 axis = {0.0f, 0.0f, 1.0f};
	float angle = glm::radians(0.0f);
};


struct Scene
{
	Scene();

	static constexpr size_t missing_material_idx = 0u;

	struct TextureResult
	{
		uint32_t texture_object = 0;
		int      num_channels   = 0;
	};

	std::map<std::string, TextureResult> texture_instances;
	std::map<std::string, uint32_t> material_instances;
	std::vector<Material>  materials;
	std::vector<Mesh>      meshes;
	std::vector<Instance>  instances;
	std::vector<PointLight> lights;

	TextureResult load_texture(std::string_view name, std::string_view basedir = std::string_view{});
	void load_model(std::string_view name, std::string_view basedir);
	size_t add_material(std::string_view name, Material&& mat, std::string_view basedir = std::string_view{});

	Camera main_camera;

	DirectionalLight directional_light {
		glm::vec3(0.2f, 1.0f, 0.5f),       // dir
		glm::vec3(0.004f, 0.009f, 0.011f), // amb
		glm::vec3(0.1f, 0.1f, 0.1f),       // diff
		glm::vec3(0.05f, 0.05f, 0.05f)};      // spec


};
