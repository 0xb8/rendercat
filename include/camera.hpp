#pragma once

#include <common.hpp>
#if 0
struct Plane
{
	glm::vec3 normal;
	float d;

	Plane() = default;

	Plane(const glm::vec3 &_normal, const glm::vec3 &_point )
	{
		normal = glm::normalize(_normal);
		d = -glm::dot(normal, _point);
	}

	Plane(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) // 3 points
	{
		normal = glm::normalize(glm::cross(b  - a, c - a));
		d = -glm::dot(normal, a);
	}

	Plane(float a, float b, float c, float d ) // 4 coefficients
	{
		// set the normal vector
		normal = glm::vec3(a, b, c);
		//compute the lenght of the vector
		float l = glm::length(normal);
		// normalize the vector
		normal = glm::vec3(a/l, b/l, c/l);
		// and divide d by th length as well
		this->d = d/l;
	}

	float distance(const glm::vec3 & p) const {
		return d + glm::dot(normal, p);
	}
};

#endif


struct Camera
{
	friend class Scene;
	struct Frustum
	{
		glm::vec4 planes[6];
		bool locked = false;

		enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };

		void update(const glm::mat4& matrix) noexcept
		{
			planes[LEFT].x  = matrix[0].w + matrix[0].x;
			planes[LEFT].y  = matrix[1].w + matrix[1].x;
			planes[LEFT].z  = matrix[2].w + matrix[2].x;
			planes[LEFT].w  = matrix[3].w + matrix[3].x;

			planes[RIGHT].x = matrix[0].w - matrix[0].x;
			planes[RIGHT].y = matrix[1].w - matrix[1].x;
			planes[RIGHT].z = matrix[2].w - matrix[2].x;
			planes[RIGHT].w = matrix[3].w - matrix[3].x;

			planes[TOP].x   = matrix[0].w - matrix[0].y;
			planes[TOP].y   = matrix[1].w - matrix[1].y;
			planes[TOP].z   = matrix[2].w - matrix[2].y;
			planes[TOP].w   = matrix[3].w - matrix[3].y;

			planes[BOTTOM].x= matrix[0].w + matrix[0].y;
			planes[BOTTOM].y= matrix[1].w + matrix[1].y;
			planes[BOTTOM].z= matrix[2].w + matrix[2].y;
			planes[BOTTOM].w= matrix[3].w + matrix[3].y;

			planes[BACK].x  = matrix[0].w + matrix[0].z;
			planes[BACK].y  = matrix[1].w + matrix[1].z;
			planes[BACK].z  = matrix[2].w + matrix[2].z;
			planes[BACK].w  = matrix[3].w + matrix[3].z;

			planes[FRONT].x = matrix[0].w - matrix[0].z;
			planes[FRONT].y = matrix[1].w - matrix[1].z;
			planes[FRONT].z = matrix[2].w - matrix[2].z;
			planes[FRONT].w = matrix[3].w - matrix[3].z;

			for (auto i = 0; i < 6; i++) {
				float length = sqrtf(planes[i].x * planes[i].x +
				                     planes[i].y * planes[i].y +
				                     planes[i].z * planes[i].z);
				planes[i] /= length;
			}
		}

		int half_plane_test(const glm::vec3 &p, const glm::vec3 &normal, float offset) const noexcept
		{
			float dist = glm::dot( p, normal ) + offset;
			if(dist > 0.02) // Point is in front of plane
				return 1;

			else if(dist < -0.02) // Point is behind plane
				return 0;

			return 2; // Point is on plane
		}
	};


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
		update_view();

	}

	void forward(float speed)
	{
		pos += speed * front;
		update_view();
	}

	void backward(float speed)
	{
		pos -= speed * front;
		update_view();
	}

	void left(float speed)
	{
		pos -= glm::normalize(glm::cross(front, up)) * speed;
		update_view();

	}

	void right(float speed)
	{
		pos += glm::normalize(glm::cross(front, up)) * speed;
		update_view();

	}

	void zoom(float newfov)
	{
		fov = newfov;
		projection = make_reversed_z_projection(glm::radians(fov), aspect, znear);
	}

	static glm::mat4 make_reversed_z_projection(float fovY_radians, float aspectWbyH, float zNear) noexcept
	{
		float f = 1.0f / tan(fovY_radians / 2.0f);
		return glm::mat4(f / aspectWbyH, 0.0f,  0.0f,  0.0f,
		                 0.0f,    f,  0.0f,  0.0f,
		                 0.0f, 0.0f,  0.0f, -1.0f,
		                 0.0f, 0.0f, zNear,  0.0f);
	}

	void update_view() noexcept
	{
		view = glm::lookAt(pos, pos + front, up);
	}

	void update_projection(float newaspect) noexcept
	{
		if(aspect != newaspect) {
			aspect = newaspect;
			projection = make_reversed_z_projection(glm::radians(fov), aspect, znear);
		}
	}

	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 pos   = glm::vec3(0.0f, 0.0f, 0.0f);
	float exposure = 1.0f;

private:
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);

	float pitch = 0.0f;
	float yaw = -90.0f;
	float fov = 90.0f;
	float aspect = 1.0f;
	float znear = 0.01f;
	float zfar  = 1000.0f; // not needed with reverse-z
};
