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
	float zfar  = 1000.0f;
};



struct Scene
{
	static const glm::vec3 cubePositions[10];


	Scene();


	GLuint vao = 0;
	GLuint texture = 0;

	Camera main_camera;



};
