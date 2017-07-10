#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

struct Camera
{
	glm::vec3 pos   = glm::vec3(0.5f, 0.0f, 3.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
	float fov = 60.0f;
	float znear = 0.01f;
	// float zfar  = 1000.0f; // not needed with reverse-z
};

struct Material
{
	glm::vec3 specular{0.6f, 0.6f, 0.6f};
	float shininess = 60.0f;
	GLuint diffuse = 0;
	GLuint normal = 0;
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
	Material material;
	GLuint vao = 0;
};



struct Scene
{
	static const glm::vec3 cubePositions[10];

	Scene();

	Instance box;

	Camera main_camera;

	DirectionalLight directional_light {
		glm::vec3(-0.4f, -1.0f, 0.4f),  // dir
		glm::vec3(0.1f, 0.11f, 0.13f),  // amb
		glm::vec3(0.6f, 0.5f, 0.45f),   // diff
		glm::vec3(1.0f, 1.0f, 0.85f)};  // spec


	PointLight point_light {
		glm::vec3(-1.0f, 3.0f, 2.0f), // pos
		glm::vec3(0.0f, 0.0f, 0.0f),  // amb
		glm::vec3(1.0, 0.0, 0.1),     // diff
		glm::vec3(1.0, 0.0, 0.0),     // spec
		1.0f,                         // const
		0.09f,                        // lin
		0.032f                        // quad
	};

};
