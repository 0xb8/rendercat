#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string_view>
#include <mesh.hpp>
#include <camera.hpp>
#include <shader_set.hpp>



struct Material
{
	glm::vec3 specular {0.6f, 0.6f, 0.6f};
	float shininess = 60.0f;

	GLuint diffuse = 0;
	GLuint normal = 0;


	void bind(GLuint s)
	{
		unif::v3(s,  "material.specular",  specular);
		unif::f1(s,  "material.shininess", shininess);
		if(diffuse) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuse);
			unif::i1(s, "material.diffuse", 0);
		}
		if(normal) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, normal);
			unif::i1(s, "material.normal", 1);
		}
	}
};

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct PointLight
{
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float constant = 1.0f;
	float linear;
	float quadratic;
	GLuint vao;
};


struct Instance
{
	uint32_t material_id;
	uint32_t mesh_id;

	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 axis = {0.0f, 0.0f, 1.0f};
	float angle = glm::radians(0.0f);
};


struct Scene
{
	Scene();

	std::vector<Material>  materials;
	std::vector<Mesh>      meshes;
	std::vector<Instance>  instances;


	size_t load_model(std::string_view path);
	size_t load_material(std::string_view diffuse, std::string_view normal);

	Camera main_camera;

	DirectionalLight directional_light {
		glm::vec3(-0.4f, -1.0f, 0.4f),  // dir
		glm::vec3(0.1f, 0.11f, 0.13f),  // amb
		glm::vec3(3.6f, 1.5f, 3.45f),   // diff
		glm::vec3(1.0f, 1.0f, 0.85f)};  // spec


	PointLight point_light {
		glm::vec3(0.0f, -5.0f, 3.0f), // pos
		glm::vec3(0.0f, 0.0f, 0.0f),  // amb
		glm::vec3(0.0, 1.0, 1.1),     // diff
		glm::vec3(0.0, 1.0, 1.0),     // spec
		1.0f,                         // const
		0.09f,                        // lin
		0.032f                        // quad
	};

};
