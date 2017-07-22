#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string_view>
#include <glm/gtx/hash.hpp>

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

	uint32_t vao;
	uint32_t numverts;

	Mesh(std::vector<vertex> && verts,
	     const std::vector<unsigned>& indices);

};

namespace std {

template<> struct hash<Mesh::vertex> {
	size_t operator()(const Mesh::vertex vertex) const {
		return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texcoords) << 1);
	}
};

}

