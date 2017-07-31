#pragma once

#include <common.hpp>

struct Plane
{
	glm::vec3 normal;
	float d;

	Plane() = default;

	Plane( const glm::vec3 &_normal, const glm::vec3 &_point )
	{
		normal = glm::normalize(_normal);
		d = -glm::dot( normal, _point );
	}

	Plane( const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3 ) // 3 points
	{
		normal = glm::normalize(glm::cross( p3 - p2, p1 - p2 ));
		d = -glm::dot( normal, p2 );
	}

	Plane(float a, float b, float c, float d ) // 4 coefficients
	{
		// set the normal vector
		normal = glm::vec3( a, b, c );
		//compute the lenght of the vector
		float l = glm::length(normal);
		// normalize the vector
		normal = glm::vec3( a/l, b/l, c/l );
		// and divide d by th length as well
		this->d = d/l;
	}

	float distance(const glm::vec3 & p) const {
		return d + glm::dot(normal, p);
	}
};

struct Frustum
{
	Plane planes[6];
	glm::vec3 ntl, ntr, nbl, nbr, ftl, ftr, fbl, fbr;
	float nw, nh, fw, fh;
	float fov, aspect, neard, fard;
	bool locked = false;

	enum
	{
		TOP = 0,
		BOTTOM,
		LEFT,
		RIGHT,
		NEARP,
		FARP
	};


	Frustum()
	{
		setup( 45, 1.f, 1.f, 100.f );
	}

	void setup( float _fov, float _aspect, float _near, float _far )
	{
		aspect = _aspect;
		fov = _fov;
		neard = _near;
		fard = _far;

		float tangent = (float)tan( glm::radians(fov * 0.5) ) ;
		nh = _near * tangent;
		nw = nh * _aspect;
		fh = _far * tangent;
		fw = fh * _aspect;
	}

	void update( const glm::vec3 &pos, const glm::vec3 &look, const glm::vec3 &up )
	{
		if(unlikely(locked)) return;

		glm::vec3 Z = glm::normalize( pos - look );
		glm::vec3 X = glm::normalize(glm::cross( up, Z ));
		glm::vec3 Y = glm::cross( Z, X );

		glm::vec3 nc = pos - Z * neard; // near center
		glm::vec3 fc = pos - Z * fard;  //  far center

		ntl = nc + ( Y * nh ) - ( X * nw );
		ntr = nc + ( Y * nh ) + ( X * nw );
		nbl = nc - ( Y * nh ) - ( X * nw );
		nbr = nc - ( Y * nh ) + ( X * nw );

		ftl = fc + ( Y * fh ) - ( X * fw );
		ftr = fc + ( Y * fh ) + ( X * fw );
		fbl = fc - ( Y * fh ) - ( X * fw );
		fbr = fc - ( Y * fh ) + ( X * fw );

		planes[    TOP ] = Plane(ntr,ntl,ftl);
		planes[ BOTTOM ] = Plane(nbl,nbr,fbr);
		planes[   LEFT ] = Plane(ntl,nbl,fbl);
		planes[  RIGHT ] = Plane(nbr,ntr,fbr);
		planes[  NEARP ] = Plane(ntl,ntr,nbr);
		planes[   FARP ] = Plane(ftr,ftl,fbl);
	}

	bool intersects(const AABB& aabb)
	{
		glm::vec3 corner;
		auto max = aabb.max();
		auto min = aabb.min();
		for(int i=0; i<6; i++) {

			corner.x = planes[i].normal.x > 0.0 ? max.x : min.x;
			corner.y = planes[i].normal.y > 0.0 ? max.y : min.y;
			corner.z = planes[i].normal.z > 0.0 ? max.z : min.z;

			if (planes[i].distance(corner) < 0.0) return false;
		}

		return true;
	}





};



struct Camera
{
	friend class Frustum;

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

		frustum.update(pos, front, up);
	}

	void forward(float speed)
	{
		pos += speed * front;
		frustum.update(pos, front, up);
	}

	void backward(float speed)
	{
		pos -= speed * front;
		frustum.update(pos, front, up);
	}

	void left(float speed)
	{
		pos -= glm::normalize(glm::cross(front, up)) * speed;
		frustum.update(pos, front, up);
	}

	void right(float speed)
	{
		pos += glm::normalize(glm::cross(front, up)) * speed;
		frustum.update(pos, front, up);
	}

	glm::mat4 view() const
	{
		return glm::lookAt(pos, pos + front, up);
	}

	glm::vec3 pos   = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
	float pitch = 0.0f;
	float yaw = -90.0f;
	float fov = 90.0f;
	float aspect = 1.0f;
	float znear = 0.01f;
	float zfar  = 1000.0f; // not needed with reverse-z

	Frustum frustum;
};

