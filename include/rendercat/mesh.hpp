#pragma once

#include <rendercat/common.hpp>
#include <rendercat/AABB.hpp>
#include <rendercat/material.hpp>
#include <string_view>
#include <vector>

namespace model {

	struct vertex;
	struct mesh
	{
		std::string name;
		gl::GLuint ebo = 0;
		gl::GLuint dynamic_vbo = 0;
		gl::GLuint static_vbo = 0;
		gl::GLuint vao = 0;
		gl::GLuint numverts = 0;
		gl::GLuint numverts_unique = 0;
		gl::GLenum   index_type{};
		AABB aabb;

		bool valid() const noexcept
		{
			return numverts != 0 && vao != 0;
		}

		mesh(const std::string & name_, std::vector<vertex>&& verts);
		~mesh();

		mesh(const mesh&) = delete;
		mesh& operator=(const mesh&) = delete;

		mesh(mesh&& o) noexcept
		{
			this->operator=(std::move(o));
		}

		mesh& operator=(mesh&& o) noexcept
		{
			assert(this != &o);
			name = std::move(o.name);
			std::swap(ebo, o.ebo);
			std::swap(dynamic_vbo, o.dynamic_vbo);
			std::swap(static_vbo, o.static_vbo);
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

