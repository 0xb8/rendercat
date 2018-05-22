#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <rendercat/mesh.hpp>
#include <rendercat/material.hpp>
#include <tiny_obj_loader.h>
#include <mikktspace.h>
#include <fmt/core.h>
#include <numeric>
#include <utility>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;
using namespace rc;

namespace rc {
namespace model {
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texcoords;

		bool operator==(const Vertex& other) const noexcept {
			return position == other.position
					&& normal == other.normal
					&& texcoords == other.texcoords;
		}
	};
}
}

static Material load_obj_material(const tinyobj::material_t& mat, const std::string_view material_path)
{
	Material material(mat.name);

	auto spec_color = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
	auto diff_color = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], mat.dissolve);
	material.set_specular_color_shininess(spec_color, mat.shininess);
	material.set_diffuse_color(diff_color);

	if(!mat.diffuse_texname.empty()) {
		bool has_alpha_mask = !mat.alpha_texname.empty();
		material.add_diffuse_map(mat.diffuse_texname, material_path);
		if(material.alpha_mode() == Texture::AlphaMode::Unknown) {
			if(has_alpha_mask) {
				material.set_alpha_mode(Texture::AlphaMode::Mask);
			} else {
				material.set_alpha_mode(Texture::AlphaMode::Blend);
			}
		}
	}

	if(!mat.normal_texname.empty()) {
		material.add_normal_map(mat.normal_texname, material_path);
	}

	if(!mat.specular_texname.empty()) {
		material.add_specular_map(mat.specular_texname, material_path);
	}

	if(!mat.metallic_texname.empty()) {
		// TODO
	}

	if(!mat.roughness_texname.empty()) {
		// TODO
	}

	static const std::string cull("cullface");
	if(mat.unknown_parameter.find(cull) != mat.unknown_parameter.end()) {
		const auto val = mat.unknown_parameter.at(cull);
		if(val == "0") {
			material.face_culling_enabled = false;
		}
	}

	return material;
}

bool model::load_obj_file(data * res, const std::string_view name, const std::string_view basedir)
{
	assert(res != nullptr);
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> obj_materials;
	std::vector<Material> scene_materials;
	std::vector<model::Mesh> scene_meshes;
	std::vector<int> scene_mesh_material;
	std::string err;

	std::string model_path{rc::path::asset::model};
	model_path.append(basedir);

	std::string model_path_full = model_path;
	model_path_full.append(name);

	std::string material_path{rc::path::asset::model_material};
	material_path.append(basedir);

	fmt::print(stderr, "\n--- loading model '{}' from '{}' ------------------------\n",
	                   name,model_path);
	size_t vertex_count = 0, unique_vertex_count = 0;


	if(!tinyobj::LoadObj(&attrib, &shapes, &obj_materials, &err, model_path_full.data(), model_path.data())) {
		fmt::print(stderr, "Error loading OBJ: {}", err);
		return false;
	}

	const bool has_normals = !attrib.normals.empty();
	const bool has_texcoords = !attrib.texcoords.empty();
	if(unlikely(!has_normals)) {
		fmt::print(stderr, "Error loading OBJ: '{}' does not have vertex normals!\n",
		           name);
		return false;
	}
	if(unlikely(!has_texcoords)) {
		fmt::print(stderr, "Error loading OBJ: '{}' does not have texture coords!\n",
		           name);
		return false;
	}

	scene_materials.reserve(obj_materials.size());
	scene_meshes.reserve(shapes.size());
	scene_mesh_material.reserve(shapes.size());

	for(const tinyobj::shape_t& shape : shapes) {
		fmt::print(stderr, " * loading submesh '{}'\n", shape.name);

		int shape_material_id = -1;

		if(unlikely(shape.mesh.material_ids.size() == 0)) {
			fmt::print(stderr, "   - missing material!\n");
		} else if(!std::equal(std::next(std::cbegin(shape.mesh.material_ids)),
		                      std::cend(shape.mesh.material_ids),
		                      std::cbegin(shape.mesh.material_ids)))
		{
			fmt::print(stderr, "   -  per-face materials detected\n");
		} else  {
			auto shape_mat_id = shape.mesh.material_ids[0];

			if(unlikely(shape_mat_id < 0 || shape_mat_id >= (int)obj_materials.size())) {
				fmt::print(stderr, "   - invalid material id:  {}\n", shape_mat_id);
			} else {
				tinyobj::material_t& mat = obj_materials[shape_mat_id];
				const auto& mat_name = mat.name;

				auto pos = std::find_if(scene_materials.cbegin(), scene_materials.cend(), [&mat_name](const auto& material)
				{
					return material.name == mat_name;
				});

				if(pos == scene_materials.cend()) {
					scene_materials.emplace_back(load_obj_material(mat, material_path));
					shape_material_id = scene_materials.size()-1;
				} else {
					shape_material_id = std::distance(scene_materials.cbegin(), pos);
				}
			}
		}

		std::vector<Vertex> vertices;
		vertices.reserve(shape.mesh.indices.size());

		for(const tinyobj::index_t& index : shape.mesh.indices) {
			Vertex vert;
			vert.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vert.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vert.texcoords = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				attrib.texcoords[2 * index.texcoord_index + 1]
			};


			vertices.push_back(vert);

		}

		model::Mesh mesh(shape.name, std::move(vertices));

		vertex_count += mesh.numverts;
		unique_vertex_count += mesh.numverts_unique;

		if(unlikely(!mesh.valid())) {
			fmt::print(stderr, "\nError loading submesh [{}]\n", shape.name);
			std::fflush(stderr);
			return false;
		}

		scene_meshes.emplace_back(std::move(mesh));

		scene_mesh_material.push_back(shape_material_id);
		if(unlikely(shape_material_id < 0)) {
			fmt::print(stderr, " - invalid material!\n");
		}

	}
	fmt::print(stderr, " ~ final vertex count: {}, unique: {} ({}% saved)\n"
	                   "\n--- model '{}' loaded -------------------------------\n",
	           vertex_count,
	           unique_vertex_count,
	           100-rc::math::percent(unique_vertex_count, vertex_count),
	           name);

	for(auto& m : scene_materials) {
		m.name.insert(0, basedir);
	}
	res->materials = std::move(scene_materials);
	res->submeshes = std::move(scene_meshes);
	res->submesh_material = std::move(scene_mesh_material);
	std::fflush(stderr);

	return true;
}

namespace {

struct tangent_sign
{
	glm::vec3 tangent;
	float sign;

	bool operator==(const tangent_sign& other) const noexcept
	{
		return tangent == other.tangent && sign == other.sign;
	}
};

struct vertex_tangent
{
	model::Vertex vertex;
	tangent_sign tangent;

	bool operator==(const vertex_tangent& other) const noexcept
	{
		return vertex == other.vertex && tangent == other.tangent;
	}
};

struct interface_data
{
	const model::Vertex* vertices;
	tangent_sign* tangents;
	uint32_t numfaces;
};

auto get_data(const SMikkTSpaceContext* ctx)
{
	return reinterpret_cast<interface_data*>(ctx->m_pUserData);
}

std::vector<tangent_sign> calculateMikktSpace(const std::vector<model::Vertex>& verts)
{
	assert(verts.size() % 3 == 0);

	std::vector<tangent_sign> tangents;
	tangents.resize(verts.size());

	interface_data idata;
	idata.numfaces = verts.size() / 3;
	idata.vertices = verts.data();
	idata.tangents = tangents.data();

	SMikkTSpaceInterface interface;
	memset(&interface, 0, sizeof(SMikkTSpaceInterface));



	interface.m_getNumFaces = [](const SMikkTSpaceContext* ctx) -> int
	{
		return get_data(ctx)->numfaces;
	};

	interface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext*, const int) -> int
	{
		return 3;
	};

	interface.m_getPosition = [](const SMikkTSpaceContext* ctx,
		float* out,
		const int face,
		const int vert) -> void
	{
		auto verts = get_data(ctx)->vertices;
		auto pos = verts[face*3+vert].position;

		out[0] = pos.x;
		out[1] = pos.y;
		out[2] = pos.z;
	};

	interface.m_getNormal = [](const SMikkTSpaceContext* ctx,
		float* out,
		const int face,
		const int vert) -> void
	{
		auto verts = get_data(ctx)->vertices;
		auto norm = verts[face*3+vert].normal;

		out[0] = norm.x;
		out[1] = norm.y;
		out[2] = norm.z;
	};

	interface.m_getTexCoord = [](const SMikkTSpaceContext* ctx,
		float* out,
		const int face,
		const int vert) -> void
	{
		auto verts = get_data(ctx)->vertices;
		auto uv = verts[face*3+vert].texcoords;

		out[0] = uv.x;
		out[1] = uv.y;
	};

	interface.m_setTSpaceBasic = [](const SMikkTSpaceContext* ctx,
		const float fvTangent[],
		const float fSign,
		const int face,
		const int vert) -> void
	{
		auto tangents = get_data(ctx)->tangents;
		tangents[face*3+vert].tangent = glm::vec3(fvTangent[0],fvTangent[1],fvTangent[2]);
		tangents[face*3+vert].sign = fSign;
	};


	SMikkTSpaceContext context;
	context.m_pInterface = &interface;
	context.m_pUserData = &idata;

	if(!genTangSpaceDefault(&context)) {
		throw std::runtime_error("Error calculating tangent space!");
	}

	return tangents;
}

} // anon namespace

namespace std {
	template<>
	struct hash<vertex_tangent>
	{
		size_t operator()(const vertex_tangent& vt) const noexcept
		{
			return hash<glm::vec3>()(vt.vertex.position)
				^ hash<glm::vec3>()(vt.vertex.normal)
				^ hash<glm::vec2>()(vt.vertex.texcoords)
				^ hash<glm::vec3>()(vt.tangent.tangent)
				^ hash<float>()(vt.tangent.sign);
		}
	};
}

model::Mesh::Mesh(const std::string& name_, std::vector<Vertex> && verts) : name(name_)
{
	auto tangents = calculateMikktSpace(verts);
	assert(verts.size() == tangents.size());
	uint32_t initial_vertex_count = verts.size();
	uint32_t unique_vertex_count = verts.size();
	uint32_t max_idx{0};

	// index the mesh ------------------------------------------------------
	std::vector<uint32_t> indices;
	std::vector<uint16_t> small_indices;

	bool use_small_indices;
	if(verts.size() < std::numeric_limits<uint16_t>::max()) {
		small_indices.reserve(verts.size());
		use_small_indices = true;
	} else {
		indices.reserve(verts.size());
		use_small_indices = false;
	}

	auto add_index = [use_small_indices,&indices,&small_indices,&max_idx](uint32_t idx)
	{
		if(likely(use_small_indices)) {
			assert(idx < std::numeric_limits<uint16_t>::max());
			small_indices.push_back(idx);
		} else {
			indices.push_back(idx);
		}
		if(idx > max_idx)
			max_idx = idx;
	};

	std::vector<vertex_tangent> vtans;
	std::unordered_map<vertex_tangent, uint32_t> uniqueVerts;
	vtans.reserve(verts.size() / 3);
	uniqueVerts.reserve(vtans.capacity());

	for(uint32_t i = 0; i < verts.size(); ++i) {
		bbox.include(verts[i].position);

		vertex_tangent vt{verts[i], tangents[i]};

		auto pos = uniqueVerts.find(vt);
		if(pos == uniqueVerts.end()) {
			auto res = uniqueVerts.insert(std::make_pair(vt, vtans.size()));
			vtans.push_back(vt);
			pos = res.first;
		}
		add_index(pos->second);
	}
	uniqueVerts.clear();
	verts.clear();
	tangents.clear();
	assert(indices.size() == initial_vertex_count || small_indices.size() == initial_vertex_count);
	unique_vertex_count = vtans.size();


	// prepare data for submission to the GPU ------------------------------
	struct dynamic_attrib
	{
		glm::vec3 position;
	};

	struct static_attrib
	{
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 texcoords; // texcoords.z == bitangent sign
	};

	std::vector<dynamic_attrib> dynamic_attribs;
	std::vector<static_attrib> static_attribs;

	dynamic_attribs.reserve(unique_vertex_count);
	static_attribs.reserve(unique_vertex_count);

	for(const auto& vt : vtans) {
		const auto& position  = vt.vertex.position;
		const auto& normal    = vt.vertex.normal;
		const auto& tangent   = vt.tangent.tangent;
		const auto& sign      = vt.tangent.sign;
		const auto& texcoords = vt.vertex.texcoords;
		auto texcoord_sign    = glm::vec3(texcoords, sign);

		dynamic_attribs.emplace_back(dynamic_attrib{position});
		static_attribs.emplace_back(static_attrib{normal, tangent, texcoord_sign});
	}
	assert(dynamic_attribs.size() == unique_vertex_count && static_attribs.size() == unique_vertex_count);
	vtans.clear();


	// set up GPU objects --------------------------------------------------
	glCreateVertexArrays(1, vao.get());

	glCreateBuffers(1, ebo.get());
	if(likely(use_small_indices)) { // reduces index memory usage by 2x for most of the meshes
		glNamedBufferStorage(*ebo, small_indices.size() * sizeof(uint16_t), small_indices.data(), GL_NONE_BIT);
		index_type = uint32_t(GL_UNSIGNED_SHORT);
	} else {
		glNamedBufferStorage(*ebo, indices.size() * sizeof(uint32_t), indices.data(), GL_NONE_BIT);
		index_type = uint32_t(GL_UNSIGNED_INT);
	}
	glVertexArrayElementBuffer(*vao, *ebo);

	glCreateBuffers(1, dynamic_vbo.get());
	glNamedBufferStorage(*dynamic_vbo, dynamic_attribs.size() * sizeof(dynamic_attrib), dynamic_attribs.data(), GL_NONE_BIT);

	glCreateBuffers(1, static_vbo.get());
	glNamedBufferStorage(*static_vbo, static_attribs.size() * sizeof(static_attrib), static_attribs.data(), GL_NONE_BIT);

	const GLuint dynamic_vbo_id = 0;
	const GLuint static_vbo_id  = 1;

	// set up VBO: binding index, vbo, offset, stride
	glVertexArrayVertexBuffer(*vao, dynamic_vbo_id, *dynamic_vbo,  0, sizeof(dynamic_attrib));
	glVertexArrayVertexBuffer(*vao, static_vbo_id,  *static_vbo,   0, sizeof(static_attrib));

	glEnableVertexArrayAttrib(*vao, 0);
	glEnableVertexArrayAttrib(*vao, 1);
	glEnableVertexArrayAttrib(*vao, 2);
	glEnableVertexArrayAttrib(*vao, 3);

	// specify attrib format: attrib idx, element count, format, normalized, relative offset
	glVertexArrayAttribFormat(*vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(dynamic_attrib, position));
	glVertexArrayAttribFormat(*vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(static_attrib,  normal));
	glVertexArrayAttribFormat(*vao, 2, 3, GL_FLOAT, GL_FALSE, offsetof(static_attrib,  tangent));
	glVertexArrayAttribFormat(*vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(static_attrib,  texcoords));

	// assign VBOs to attributes
	glVertexArrayAttribBinding(*vao, 0, dynamic_vbo_id);
	glVertexArrayAttribBinding(*vao, 1, static_vbo_id);
	glVertexArrayAttribBinding(*vao, 2, static_vbo_id);
	glVertexArrayAttribBinding(*vao, 3, static_vbo_id);

	if(glGetError() != GL_NO_ERROR)
		return;

	// mark submesh as valid -----------------------------------------------
	numverts = initial_vertex_count;
	numverts_unique = unique_vertex_count;
	index_max = max_idx;
	index_min = 0;
	if(max_idx != unique_vertex_count - 1)
		fmt::print(stderr, "[submesh] inconsistent indices detected!\n");
}
