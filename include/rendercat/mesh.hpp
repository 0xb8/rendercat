#pragma once

#include <rendercat/common.hpp>
#include <rendercat/core/bbox.hpp>
#include <rendercat/material.hpp>
#include <rendercat/util/gl_unique_handle.hpp>
#include <string_view>
#include <vector>
#include <zcm/quat.hpp>

namespace rc {
namespace model {

	struct attr_description_t;

	struct Mesh
	{
		std::string name;
		buffer_handle ebo;
		buffer_handle vbo;
		vertex_array_handle vao;
		uint32_t numverts = 0;
		uint32_t numverts_unique = 0;
		uint32_t index_min = 0xFFFFFFFF;
		uint32_t index_max = 0;
		uint32_t touched_lights = 0;
		uint32_t index_type{};
		uint32_t draw_mode{};
		bbox3 bbox;
		bool has_tangents = false;

		bool valid() const noexcept;

		explicit Mesh(std::string name_);
		~Mesh() = default;

		void upload_data(attr_description_t index, std::vector<attr_description_t> attrs);

		RC_DEFAULT_MOVE_NOEXCEPT(Mesh)
		RC_DISABLE_COPY(Mesh)
	};

	struct data
	{
		std::vector<Material>  materials;
		std::vector<Mesh>      primitives;
		std::vector<uint32_t>  primitive_material;
		std::vector<zcm::quat> rotation;
		std::vector<zcm::vec3> scale;
		std::vector<zcm::vec3> translate;
	};

	bool load_gltf_file(data& res,
	                    const std::string_view name,
			    const std::string_view basedir = std::string_view{});
} // namespace model
} // namespace rc

