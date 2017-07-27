#pragma once

#include <common.hpp>

struct Camera
{
	void aim(float dx, float dy)
	{
		yaw += dx;
		pitch += dy;
		pitch = glm::clamp(pitch, -89.9f, 89.9f);
		yaw = fmod(yaw, 360.0);
		if(yaw < 0)
		    yaw += 360.0;

		front = glm::normalize(glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
		                                 sin(glm::radians(pitch)),
		                                 sin(glm::radians(yaw)) * cos(glm::radians(pitch))));
	}

	void forward(float speed)
	{
		pos += speed * front;
	}

	void backward(float speed)
	{
		pos -= speed * front;
	}

	void left(float speed)
	{
		pos -= glm::normalize(glm::cross(front, up)) * speed;
	}

	void right(float speed)
	{
		pos += glm::normalize(glm::cross(front, up)) * speed;
	}

	glm::vec3 pos   = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
	float pitch = 0.0f;
	float yaw = -90.0f;
	float fov = 90.0f;
	float znear = 0.01f;
	// float zfar  = 1000.0f; // not needed with reverse-z
};
