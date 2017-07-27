#include <GL/glew.h>
#include <mesh.hpp>
//#include <iostream>

static void* gloffset(unsigned off)
{
	return reinterpret_cast<void*>(off);
}

Mesh::Mesh(std::vector<vertex>&& verts,
           std::vector<uint32_t>&& indices)
{
	// calculate tangents and bitangents
	for(uint32_t i = 0; i < indices.size(); i += 3) {
		auto& vert0 = verts[indices[i]];
		auto& vert1 = verts[indices[i+1]];
		auto& vert2 = verts[indices[i+2]];

		const auto v0 = vert0.position;
		const auto v1 = vert1.position;
		const auto v2 = vert2.position;

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

		vert0.bitangent += binormal;
		vert1.bitangent += binormal;
		vert2.bitangent += binormal;
	}

	for(uint32_t i = 0; i < indices.size(); ++i) {
		auto& vert = verts[indices[i]];
		// re-orthogonalize T with N
		vert.tangent = glm::normalize(vert.tangent - vert.normal * glm::dot(vert.normal, vert.tangent));
		if(glm::dot(cross(vert.normal, vert.tangent), vert.bitangent) < 0.0f) {
			vert.tangent *= -1.0f;
		}
	}

	GLuint vbo, ebo;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vertex), verts.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, position)));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, normal)));

	glEnableVertexAttribArray(2); // NOTE: size of 2 below is very important!
	glVertexAttribPointer(2, 2,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, texcoords)));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, tangent)));

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3,  GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, bitangent)));

	glBindVertexArray(0);

	numverts = indices.size();
}
