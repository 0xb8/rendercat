#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/AABB.hpp>
#include <rendercat/material.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <string_view>
#include <vector>

namespace model {

	struct vertex;
	struct mesh
	{
		std::string name;
		rc::buffer_handle ebo;
		rc::buffer_handle dynamic_vbo;
		rc::buffer_handle static_vbo;
		rc::vertex_array_handle vao;
		gl::GLuint numverts = 0;
		gl::GLuint numverts_unique = 0;
		gl::GLuint index_min = 0; // always 0 for now
		gl::GLuint index_max = 0;
		gl::GLuint touched_lights = 0;
		gl::GLenum index_type{};
		rc::AABB aabb;

		bool valid() const noexcept
		{
			return numverts != 0 && vao;
		}

		mesh(const std::string & name_, std::vector<vertex>&& verts);
		~mesh() = default;

		mesh(mesh&& o) noexcept = default;
		mesh& operator=(mesh&& o) noexcept = default;
		RC_DISABLE_COPY(mesh)
	};

	struct data
	{
		std::vector<Material> materials;
		std::vector<mesh>     submeshes;
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

