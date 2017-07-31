#include <GL/glew.h>
#include <mesh.hpp>
#include <AABB.hpp>
//#include <iostream>

static void* gloffset(unsigned off)
{
	return reinterpret_cast<void*>(off);
}

Mesh::Mesh(std::vector<vertex>&& verts,
           std::vector<uint32_t>&& indices)
{
	assert(indices.size() % 3 == 0);
	std::vector<glm::vec3> bitangents;
	bitangents.resize(verts.size());

	// calculate tangents and bitangents
	for(uint32_t i = 0; i < indices.size(); i += 3) {
		auto& vert0 = verts[indices[i]];
		auto& vert1 = verts[indices[i+1]];
		auto& vert2 = verts[indices[i+2]];

		const auto v0 = vert0.position;
		const auto v1 = vert1.position;
		const auto v2 = vert2.position;
		aabb.include(v0);
		aabb.include(v1);
		aabb.include(v2);

		const auto uv0  = vert0.texcoords;
		const auto uv1  = vert1.texcoords;
		const auto uv2  = vert2.texcoords;

		const auto edge1 = v1 - v0;
		const auto edge2 = v2 - v0;
		const auto d_uv1 = uv1 - uv0;
		const auto d_uv2 = uv2 - uv0;

		float f = 1.0f / (d_uv1.x * d_uv2.y - d_uv1.y * d_uv2.x);

		glm::vec3 tangent  = f * (edge1 * d_uv2.y - edge2 * d_uv1.y);
		glm::vec3 binormal = f * (edge2 * d_uv1.x - edge1 * d_uv2.x);

		vert0.tangent += tangent;
		vert1.tangent += tangent;
		vert2.tangent += tangent;

		bitangents[indices[i]]   += binormal;
	}

	for(uint32_t i = 0; i < indices.size(); i += 3) {
		auto& vert0 = verts[indices[i]];
		auto& vert1 = verts[indices[i+1]];
		auto& vert2 = verts[indices[i+2]];
		const auto n = vert0.normal;
		auto t = vert0.tangent;
		const auto b = bitangents[indices[i]];

		// re-orthogonalize T with N
		t = glm::normalize(t - n * glm::dot(n, t));
		// fix sign
		if(glm::dot(cross(n, t), b) < 0.0f) {
			t = -t;
		}

		vert0.tangent = t;
		vert1.tangent = t;
		vert2.tangent = t;
	}

	GLuint vbo, ebo;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

#if USE_MUTABLE_STORAGE_FOR_VERTICES
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vertex), verts.data(), GL_STATIC_DRAW);
#else
	glBufferStorage(GL_ARRAY_BUFFER, verts.size() * sizeof(vertex), verts.data(), 0);
#endif

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

#if USE_MUTABLE_STORAGE_FOR_VERTICES
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
#else
	glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), 0);
#endif

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, position)));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, normal)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, tangent)));

	glEnableVertexAttribArray(3); // NOTE: size of 2 below is very important!
	glVertexAttribPointer(3, 2,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, texcoords)));

	//glEnableVertexAttribArray(4);
	//glVertexAttribPointer(4, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, bitangent)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	numverts = indices.size();
}
