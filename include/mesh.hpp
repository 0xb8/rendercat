#pragma once

#include <common.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <string_view>
#include <AABB.hpp>
#include <material.hpp>

namespace model {

	struct mesh
	{
		struct vertex {
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec3 tangent;
			glm::vec3 bitangent;
			glm::vec2 texcoords;

			bool operator==(const vertex& other) const noexcept {
				return position == other.position && normal == other.normal && texcoords == other.texcoords;
			}
		};

		std::string name;
		uint32_t ebo = 0;
		uint32_t mut_vbo = 0;
		uint32_t imut_vbo = 0;
		uint32_t vao = 0;
		uint32_t numverts = 0;
		uint32_t numverts_unique = 0;
		GLenum   index_type = 0;
		AABB aabb;

		mesh(const std::string & name_,
		     std::vector<vertex>&& verts,
		     std::vector<uint32_t>&& indices);

		~mesh();

		mesh(const mesh&) = delete;
		mesh& operator=(const mesh&) = delete;

		mesh(mesh&& o) noexcept
		{
			this->operator=(std::move(o));
		}

		mesh& operator=(mesh&& o) noexcept
		{
			name = std::move(o.name);
			std::swap(ebo, o.ebo);
			std::swap(mut_vbo, o.mut_vbo);
			std::swap(imut_vbo, o.imut_vbo);
			std::swap(vao, o.vao);
			numverts = o.numverts;
			numverts_unique = o.numverts_unique;
			index_type = o.index_type;
			aabb = o.aabb;
			return *this;
		}
	};

	struct data
	{
		std::vector<Material> materials;
		std::vector<mesh> submeshes;
		std::vector<int> submesh_material;
	};


	bool load_obj_file(data* res,
			   const std::string_view name,
			   const std::string_view basedir = std::string_view{});

	// TODO: gltf loader
	bool load_gltf_file(data* res,
	                    const std::string_view name,
			    const std::string_view basedir = std::string_view{});
} // namespace model

