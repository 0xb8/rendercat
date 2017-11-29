#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/AABB.hpp>
#include <rendercat/material.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <string_view>
#include <vector>

namespace rc {
namespace model {

	struct Vertex;
	struct Mesh
	{
		std::string name;
		buffer_handle ebo;
		buffer_handle dynamic_vbo;
		buffer_handle static_vbo;
		vertex_array_handle vao;
		uint32_t numverts = 0;
		uint32_t numverts_unique = 0;
		uint32_t index_min = 0; // always 0 for now
		uint32_t index_max = 0;
		uint32_t touched_lights = 0;
		uint32_t index_type{};
		AABB aabb;

		bool valid() const noexcept
		{
			return numverts != 0 && vao;
		}

		Mesh(const std::string & name_, std::vector<Vertex>&& verts);
		~Mesh() = default;

		RC_DEFAULT_MOVE_NOEXCEPT(Mesh)
		RC_DISABLE_COPY(Mesh)
	};

	struct data
	{
		std::vector<Material> materials;
		std::vector<Mesh>     submeshes;
		std::vector<int>      submesh_material;
	};


	bool load_obj_file(data* res,
			   const std::string_view name,
			   const std::string_view basedir = std::string_view{});

	// TODO: gltf loader
	bool load_gltf_file(data* res,
	                    const std::string_view name,
			    const std::string_view basedir = std::string_view{});
} // namespace model
} // namespace rc

