#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>


struct Scene
{
	Scene();


	GLuint vao = 0;
	GLuint texture = 0;

	glm::vec3 cameraPos   = glm::vec3(0.5f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
	float fov = 60.0f;

};
