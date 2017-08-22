#pragma once

#include <common.hpp>
#include <string_view>
#include <map>
#include <mesh.hpp>
#include <camera.hpp>
#include <material.hpp>
#include <point_light.hpp>
#include <cubemap.hpp>
#include <memory>

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

	bool window_shown = true;
	int   desired_msaa_level = 0;
	float desired_render_scale = 1.0f;

	void update();

	void load_model(const std::string_view name, const std::string_view basedir);

	Camera main_camera;

	DirectionalLight directional_light {
		glm::vec3(0.3f, 1.0f, 0.8f),       // dir
		glm::vec3(0.015f, 0.01f, 0.019f), // amb
		glm::vec3(0.15f, 0.15f, 0.12f),       // diff
		glm::vec3(0.065f, 0.055f, 0.055f)};    // spec

	Cubemap cubemap;
};
