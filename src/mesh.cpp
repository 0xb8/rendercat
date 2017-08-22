#include <GL/glew.h>
#include <mesh.hpp>
#include <tiny_obj_loader.h>
#include <material.hpp>
#include <iostream>

namespace std {
	template<>
	struct hash<model::mesh::vertex>
	{
		size_t operator()(const model::mesh::vertex& vertex) const noexcept
		{
			return hash<glm::vec3>()(vertex.position)
				^ hash<glm::vec3>()(vertex.normal)
				^ hash<glm::vec2>()(vertex.texcoords);
		}
	};
}

static Material load_obj_material(const tinyobj::material_t& mat, const std::string_view material_path)
{
	Material material(mat.name);

	auto spec_color = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
	material.specularColorShininess(spec_color, mat.shininess);

	if(!mat.diffuse_texname.empty()) {
		std::cerr << "   ~ loading diffuse [" << mat.diffuse_texname << "]...   ";
		bool has_alpha_mask = !mat.alpha_texname.empty();
		material.addDiffuseMap(mat.diffuse_texname, material_path, has_alpha_mask);
		std::cerr << '\n';
	} else {
		std::cerr << "   - MISSING diffuse map\n";
	}

	if(!mat.normal_texname.empty()) {
		std::cerr << "   ~ loading normal [" << mat.normal_texname << "]...   ";
		material.addNormalMap(mat.normal_texname, material_path);
		std::cerr << '\n';
	}

	if(!mat.specular_texname.empty()) {
		std::cerr << "   ~ loading specular [" << mat.specular_texname << "]...   ";
		material.addSpecularMap(mat.specular_texname, material_path);
		std::cerr << '\n';
	}

	if(!mat.metallic_texname.empty()) {
		// TODO
	}

	if(!mat.roughness_texname.empty()) {
		// TODO
	}

	return std::move(material);
}

bool model::load_obj_file(data * res, const std::string_view name, const std::string_view basedir)
{
	assert(res != nullptr);
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> obj_materials;
	std::vector<Material> scene_materials;
	std::vector<model::mesh> scene_meshes;
	std::vector<int> scene_mesh_material;
	std::string err;

	std::string model_path{"assets/models/"};
	model_path.append(basedir);

	std::string model_path_full = model_path;
	model_path_full.append(name);

	std::string material_path{"assets/materials/models/"};
	material_path.append(basedir);

	std::cerr << "\n--- loading model \'" << name << "\' from \'" << model_path << "\' ------------------------\n";
	size_t vertex_cout = 0, unique_vertex_count = 0;


	if(!tinyobj::LoadObj(&attrib, &shapes, &obj_materials, &err, model_path_full.data(), model_path.data())) {
		std::cerr << "Error loading OBJ: " << err << std::endl;
		return false;
	}

	const bool has_normals = !attrib.normals.empty();
	const bool has_texcoords = !attrib.texcoords.empty();
	if(unlikely(!has_normals)) {
		std::cerr << "Error loading OBJ: " << name << " does not have vertex normals!\n";
		return false;
	}
	if(unlikely(!has_texcoords)) {
		std::cerr << "Error loading OBJ: " << name << " does not have texture coords!\n";
		return false;
	}

	scene_materials.reserve(obj_materials.size());
	scene_meshes.reserve(shapes.size());
	scene_mesh_material.reserve(shapes.size());

	for(const tinyobj::shape_t& shape : shapes) {
		std::cerr << " * loading submesh \'" << shape.name << "\'\n";

		int shape_material_id = -1;

		if(unlikely(shape.mesh.material_ids.size() == 0)) {
			std::cerr << "   - missing material!\n";
		} else if(!std::equal(std::next(std::cbegin(shape.mesh.material_ids)),
		                      std::cend(shape.mesh.material_ids),
		                      std::cbegin(shape.mesh.material_ids)))
		{
			std::cerr << "   -  per-face materials detected\n";
		} else  {
			auto shape_mat_id = shape.mesh.material_ids[0];

			if(unlikely(shape_mat_id < 0 || shape_mat_id >= (int)obj_materials.size())) {
				std::cerr << "   - invalid material id:  " << shape_mat_id << '\n';
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


		std::unordered_map<model::mesh::vertex, uint32_t> uniqueVertices;
		std::vector<model::mesh::vertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(shape.mesh.indices.size());
		indices.reserve(shape.mesh.indices.size());
		for(const tinyobj::index_t& index : shape.mesh.indices) {
			model::mesh::vertex vert;
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

			if (uniqueVertices.count(vert) == 0) {
				uniqueVertices.emplace(std::make_pair(vert, static_cast<uint32_t>(vertices.size())));
				vertices.push_back(vert);
			}

			indices.push_back(uniqueVertices[vert]);
		}

		vertex_cout += indices.size();
		unique_vertex_count += vertices.size();

		scene_meshes.emplace_back(model::mesh(shape.name, std::move(vertices), std::move(indices)));
		if(unlikely(scene_meshes.back().numverts == 0 || scene_meshes.back().vao == 0)) {
			std::cerr << "Error loading submesh [" << shape.name << "]\n";
			return false;
		}

		scene_mesh_material.push_back(shape_material_id);
		if(unlikely(shape_material_id < 0)) {
			std::cerr << " - invalid material!\n";
		}
	}
	std::cerr << " ~ final vertex count: "
		  << vertex_cout << ", unique: " << unique_vertex_count
		  << " (" << 100-m::percent(unique_vertex_count, vertex_cout) << "% saved)";
	std::cerr << "\n--- model \'" << name <<"\' loaded -------------------------------\n" << std::endl;

	for(auto& m : scene_materials)
		m.name.insert(0, basedir);
	res->materials = std::move(scene_materials);
	res->submeshes = std::move(scene_meshes);
	res->submesh_material = std::move(scene_mesh_material);

	return true;
}

model::mesh::mesh(const std::string& name_,
                  std::vector<vertex>&& verts,
                  std::vector<uint32_t>&& indices) : name(name_)
{
	if(unlikely(indices.size() % 3 != 0)) {
		std::cerr << "Invalid index count: " << indices.size() << ", should be divisible by 3\n";
		return;
	}

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

		// NOTE: if not clamped, some small triangles become black (not sure if upper bound is needed though)
		const auto area = (d_uv1.x * d_uv2.y - d_uv1.y * d_uv2.x);
		const auto sign = glm::sign(area);
		const auto f = sign * (1.0f / glm::max(glm::abs(area), 0.001f));

		glm::vec3 tangent   = f * (edge1 * d_uv2.y - edge2 * d_uv1.y);
		glm::vec3 bitangent = f * (edge2 * d_uv1.x - edge1 * d_uv2.x);

		vert0.tangent += tangent;
		vert1.tangent += tangent;
		vert2.tangent += tangent;

		vert0.bitangent += bitangent;
		vert0.bitangent += bitangent;
		vert0.bitangent += bitangent;
	}

	for(uint32_t i = 0; i < verts.size(); ++i) {
		auto& vert = verts[i];
		const auto n = glm::normalize(vert.normal);
		auto t = vert.tangent;
		auto b = vert.bitangent;

		if(unlikely(glm::length(t) < 1e-6f)) {
			// attempt to fix bad tangent
			t = glm::vec3(0.0f, 0.0f, 1.0f);
			if(glm::length(b) > 1e-6f)
				t = glm::normalize(t - b * glm::dot(b, t));
		}

		// re-orthogonalize T with N
		t = glm::normalize(t - n * glm::dot(n, t));
		// fix sign
		if(glm::dot(cross(n, t), b) < 0.0f) {
			t = -t;
		}
		vert.tangent = t;
		vert.normal = n;
	}

	struct mutable_attrib
	{
		glm::vec3 position;
	};

	struct immutable_attrib
	{
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoords;
	};


	std::vector<mutable_attrib> mutable_attrs;
	std::vector<immutable_attrib> immutable_attrs;
	std::vector<uint16_t> small_indices;
	bool use_small_indices = verts.size() < std::numeric_limits<uint16_t>::max();
	if(use_small_indices) {
		small_indices.reserve(indices.size());
		for(auto idx : indices) {
			if(unlikely(idx > std::numeric_limits<uint16_t>::max() || idx >= verts.size())) {
				std::cerr << "Invalid vertex index: " << idx << ", should be less than " << verts.size() << '\n';
				return;
			}
			small_indices.push_back(idx);
		}
	}

	mutable_attrs.reserve(verts.size());
	immutable_attrs.reserve(verts.size());

	for(const auto& v : verts) {
		mutable_attrs.push_back(mutable_attrib{v.position});
		immutable_attrs.push_back(immutable_attrib{v.normal, v.tangent, v.texcoords});
	}

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &ebo);

	// BUG: for some reason EXT_DSA does not provide VertexArrayElementBuffer(), only GL 4.5 ARB_DSA does.
	//----------------------------------------------------------------------
	GLint old_ebo = 0, old_vao = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &old_ebo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	if(likely(use_small_indices)) {
		glNamedBufferStorageEXT(ebo, small_indices.size() * sizeof(uint16_t), small_indices.data(), 0);
		index_type = GL_UNSIGNED_SHORT;
	} else {
		std::cerr << "Using 32-bit indices...\n";
		glNamedBufferStorageEXT(ebo, indices.size() * sizeof(uint32_t), indices.data(), 0);
		index_type = GL_UNSIGNED_INT;
	}
	glBindVertexArray(old_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, old_ebo);
	//----------------------------------------------------------------------


	glGenBuffers(1, &mut_vbo);
	glNamedBufferStorageEXT(mut_vbo, mutable_attrs.size() * sizeof(mutable_attrib), mutable_attrs.data(), 0);

	glGenBuffers(1, &imut_vbo);
	glNamedBufferStorageEXT(imut_vbo, immutable_attrs.size() * sizeof(immutable_attrib), immutable_attrs.data(), 0);

	glEnableVertexArrayAttribEXT(vao, 0);
	glVertexArrayVertexAttribOffsetEXT(vao, mut_vbo, 0, 3,  GL_FLOAT, false, sizeof(mutable_attrib),    offsetof(mutable_attrib, position));

	glEnableVertexArrayAttribEXT(vao, 1);
	glVertexArrayVertexAttribOffsetEXT(vao, imut_vbo, 1, 3,  GL_FLOAT, false, sizeof(immutable_attrib), offsetof(immutable_attrib, normal));

	glEnableVertexArrayAttribEXT(vao, 2);
	glVertexArrayVertexAttribOffsetEXT(vao, imut_vbo, 2, 3,  GL_FLOAT, false, sizeof(immutable_attrib), offsetof(immutable_attrib, tangent));

	glEnableVertexArrayAttribEXT(vao, 3); // NOTE: size of 2 below is very important!
	glVertexArrayVertexAttribOffsetEXT(vao, imut_vbo, 3, /*-->*/ 2,  GL_FLOAT, false, sizeof(immutable_attrib), offsetof(immutable_attrib, texcoords));

	if(glGetError() != GL_NO_ERROR)
		return;

	numverts = indices.size();
	numverts_unique = verts.size();
}

model::mesh::~mesh()
{
	glDeleteBuffers(1, &mut_vbo);
	glDeleteBuffers(1, &imut_vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteVertexArrays(1, &vao);
}


