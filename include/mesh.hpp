#pragma once

#include <common.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <string_view>
#include <AABB.hpp>

struct Mesh
{
	struct vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoords;

		bool operator==(const vertex& other) const noexcept {
			return position == other.position && normal == other.normal && texcoords == other.texcoords;
		}
	};

	uint32_t ebo = 0;
	uint32_t vbo = 0;
	uint32_t vao = 0;
	uint32_t numverts = 0;
	AABB aabb;

	Mesh(std::vector<vertex>&& verts, std::vector<uint32_t>&& indices);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&& o) noexcept
	{
		this->operator=(std::move(o));
	}

	Mesh& operator=(Mesh&& o) noexcept
	{
		std::swap(ebo, o.ebo);
		std::swap(vbo, o.vbo);
		std::swap(vao, o.vao);
		numverts = o.numverts;
		aabb = o.aabb;
		return *this;
	}
};

namespace std {

template<>
struct hash<Mesh::vertex>
{
	size_t operator()(const Mesh::vertex vertex) const noexcept
	{
		return hash<glm::vec3>()(vertex.position)
			^ hash<glm::vec3>()(vertex.normal)
			^ hash<glm::vec2>()(vertex.texcoords);
	}
};
}

