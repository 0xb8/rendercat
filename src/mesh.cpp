#include <mesh.hpp>
#include <GL/glew.h>
#include <tiny_obj_loader.h>
#include <iostream>

static void* gloffset(unsigned off)
{
	return reinterpret_cast<void*>(off);
}

Mesh::Mesh(std::vector<vertex>&& verts,
           const std::vector<uint32_t>& indices)
{
	for(uint32_t i = 0; i < indices.size(); i += 3) {
		auto& vert0 = verts[indices[i]];
		auto& vert1 = verts[indices[i+1]];
		auto& vert2 = verts[indices[i+2]];

		const auto vert0pos = vert0.position;
		const auto vert1pos = vert1.position;
		const auto vert2pos = vert2.position;
		const auto vert0uv  = vert0.texcoords;
		const auto vert1uv  = vert1.texcoords;
		const auto vert2uv  = vert2.texcoords;

		auto edge1 = vert1pos - vert0pos;
		auto edge2 = vert2pos - vert0pos;
		auto d_uv1 = vert1uv - vert0uv;
		auto d_uv2 = vert2uv - vert0uv;

		glm::vec3 tangent;

		float f = 1.0f / (d_uv1.x * d_uv2.y - d_uv2.x * d_uv1.y);
		tangent.x = f * (d_uv2.y * edge1.x - d_uv1.y * edge2.x);
		tangent.y = f * (d_uv2.y * edge1.y - d_uv1.y * edge2.y);
		tangent.z = f * (d_uv2.y * edge1.z - d_uv1.y * edge2.z);
		tangent = glm::normalize(tangent);


		vert0.tangent = tangent;
		vert1.tangent = tangent;
		vert2.tangent = tangent;

//		std::cerr << "TBN for poly (" << indices[i] << ',' << indices[i+1] << ',' << indices[i+2] << ")\t"
//		          << tangent.x << ' '
//		          << tangent.y << ' '
//		          << tangent.z << std::endl;
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
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, position)));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, normal)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, texcoords)));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(vertex), gloffset(offsetof(vertex, tangent)));

	glBindVertexArray(0);

	numverts = indices.size();
}
