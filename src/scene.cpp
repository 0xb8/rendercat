#include <scene.hpp>

#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <iostream>
#include <unordered_map>

Scene::TextureResult Scene::load_texture(std::string_view name, std::string_view basedir, bool linear)
{
	static const float max_aniso = 16.0f;

	std::string path(basedir);
	path.append(name);
	auto it = texture_instances.find(path);
	if(it != texture_instances.end())
	{
		return it->second;
	}

	static float maxAnisotropy = 0.0f;
	if(maxAnisotropy == 0.0f)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	TextureResult res;
	int width, height, nrChannels;
	auto data = stbi_load(path.data(), &width, &height, &nrChannels, 0);
	if (data)
	{
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(maxAnisotropy, max_aniso));

		if(nrChannels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, (linear ? GL_RGB : GL_SRGB), width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		if(nrChannels == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, (linear ? GL_RGBA : GL_SRGB_ALPHA), width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);
		res.texture_object = tex;
		res.num_channels = nrChannels;
		texture_instances.emplace(std::make_pair(std::move(path), res));
	}
	return res;
}

void Scene::load_model(std::string_view name, std::string_view basedir)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> obj_materials;
	std::string err;

	std::string model_path{"assets/models/"};
	model_path.append(basedir);

	std::string model_path_full = model_path;
	model_path_full.append(name);

	std::string material_path{"assets/materials/"};
	material_path.append(basedir);

	std::cerr << "\n--- loading model \'" << name << "\' from \'" << model_path << "\' ------------------------\n";


	if(!tinyobj::LoadObj(&attrib, &shapes, &obj_materials, &err, model_path_full.data(), model_path.data()))
		throw std::runtime_error(err);

	const bool has_normals = !attrib.normals.empty();
	const bool has_texcoords = !attrib.texcoords.empty();
	if(!has_normals) {
		std::cerr << "Model " << name << " does not have vertex normals!\n";
	}
	if(!has_texcoords) {
		std::cerr << "Model " << name << " does not have texture coords!\n";
	}

	obj_materials.push_back(tinyobj::material_t());
	for(size_t i = 0; i < obj_materials.size(); ++i) {
		const auto& mat = obj_materials[i];
		Material material;
		material.specular_color = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
		material.shininess = mat.shininess;
		std::cerr << " * material:\t\'" << mat.name << "\'\tformat_id: " << i << "\n";
		std::cerr << "   ~ material params:\tspecular:  " << material.specular_color << ",  shininess:  " << material.shininess << '\n';

		if(!mat.diffuse_texname.empty()) {
			std::cerr << "   ~ loading diffuse \'" << mat.diffuse_texname << '\'';
			auto res = load_texture(mat.diffuse_texname, material_path);

			if(res.texture_object == 0) {
				std::cerr << " \tbad texture!";
			} else {
				material.diffuse_map = res.texture_object;
				material.opaque      = (res.num_channels < 4);
				if(!material.opaque) {
					std::cerr << " translucent texture!";
				}
			}
			std::cerr << '\n';
		} else {
			std::cerr << "   - MISSING diffuse map\n";
		}

		if(!mat.normal_texname.empty()) {
			std::cerr << "   ~ loading normal \'" << mat.normal_texname << '\'';
			auto res = load_texture(mat.normal_texname, material_path, true);
			if(res.texture_object && res.num_channels >= 3) {
				material.normal_map = res.texture_object;
			} else {
				std::cerr << " not enough channels: " << res.num_channels;
			}
			std::cerr << '\n';
		}

		if(!mat.specular_texname.empty()) {
			std::cerr << "   ~ loading specular \'" << mat.specular_texname << '\'';
			auto res = load_texture(mat.specular_texname, material_path, true);
			if(res.texture_object) {
				material.specular_map = res.texture_object;
			}
			std::cerr << '\n';
		}

		auto res = add_material(mat.diffuse_texname, std::move(material), material_path);
		std::cerr << "   + ready\tformat_id:  " << i << ",  scene_id:  " << res << '\n' << std::endl;
	}

	std::cerr << "\n--- materials loaded -----------------------\n\n";

	for(const tinyobj::shape_t& shape : shapes) {

		std::cerr << " * loading submesh \'" << shape.name << "\'\n";

		Instance instance;

		int mat_id = -1;
		for(auto matid : shape.mesh.material_ids) {
			if(mat_id < 0) mat_id = matid;

			if(matid != mat_id) {
				std::cerr << "   -  per-face materials detected!  old:\t\'"
				          << obj_materials[mat_id].name
				          << "\'   new:\t\'"
				          << obj_materials[matid].name << "\'\n";
				break;
			}
		}

		if (mat_id < 0 || (mat_id >= (int)obj_materials.size())) {
			std::cerr << "   - invalid material id:  " << mat_id << '\n';
			instance.material_id = missing_material_idx;
		} else {
			tinyobj::material_t& mat = obj_materials[mat_id];
			auto pos = material_instances.find(material_path + mat.diffuse_texname);
			if(pos != material_instances.end()) {
				instance.material_id = pos->second;
				//std::cerr << "   ~ material \'" << mat.name << "\' (" << mat_id << ',' << pos->second  << ")\n";
			} else {
				std::cerr << "   - could not find diffuse " << mat.diffuse_texname  << " in materials!\n";
				instance.material_id = missing_material_idx;
			}
		}

		std::unordered_map<Mesh::vertex, uint32_t> uniqueVertices;
		std::vector<Mesh::vertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(shape.mesh.indices.size());
		indices.reserve(shape.mesh.indices.size());
		for(const tinyobj::index_t& index : shape.mesh.indices) {

			Mesh::vertex vert;

			vert.position = {
			        attrib.vertices[3 * index.vertex_index + 0],
			        attrib.vertices[3 * index.vertex_index + 1],
			        attrib.vertices[3 * index.vertex_index + 2]
			};

			if(has_normals) {
				vert.normal = {
				        attrib.normals[3 * index.normal_index + 0],
				        attrib.normals[3 * index.normal_index + 1],
				        attrib.normals[3 * index.normal_index + 2]
				};
			}

			if(has_texcoords) {
				vert.texcoords = {
				        attrib.texcoords[2 * index.texcoord_index + 0],
				        attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}
#ifndef DISABLE_COMPACTION
			if (uniqueVertices.count(vert) == 0) {
				uniqueVertices[vert] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vert);
			}

			indices.push_back(uniqueVertices[vert]);

#else
			vertices.push_back(vert);
			indices.push_back(vertices.size()-1);
#endif
		}

		//std::cerr << "   + done loading submesh,  vertices:  " << indices.size() << "  unique:  " << vertices.size() << '\n' << std::endl;
		meshes.emplace_back(Mesh(std::move(vertices), std::move(indices)));
		instance.mesh_id = meshes.size()-1;
		instances.emplace_back(std::move(instance));
	}
	std::cerr << "\n--- model \'" << name <<"\' loaded -------------------------------\n" << std::endl;
}

size_t Scene::add_material(std::string_view name, Material&& mat, std::string_view basedir)
{
	std::string path(basedir);
	path.append(name);
	materials.emplace_back(std::move(mat));
	auto pos = materials.size()-1;
	material_instances.insert(std::make_pair(path, pos));
	return pos;

}

uint32_t Material::default_diffuse = 0;
Scene::Scene()
{
	Material::set_default_diffuse(load_texture("missing.png", "assets/materials/").texture_object);

	PointLight pl;
	pl.position({8.0f, 2.0f, 2.0f})
	  .ambient({0.1f, 0.0f, 0.0f})
	  .diffuse({1.0, 0.2, 0.1})
	  .specular({0.3, 0.05, 0.0})
	  .radius(10.0)
	  .flux(150.0);

//	lights.push_back(pl);

//	pl.position({8.0f, 2.0f, -3.0f});
//	lights.push_back(pl);

	pl.position({-10.0f, 2.0f, -3.0f});;
	lights.push_back(pl);

	pl.position({-10.0f, 2.0f, 2.0f});
	lights.push_back(pl);

	pl.position({1.5f, 2.0f, 1.5f});
	lights.push_back(pl);

	pl.position({1.5f, 2.5f, -1.5f})
	  .diffuse({0.1, 0.2, 1.0})
	  .specular({0.0, 0.05, 0.3});
	lights.push_back(pl);



	stbi_set_flip_vertically_on_load(1);
	auto missing_mat_idx = add_material("missing", Material{});
	assert(missing_mat_idx == missing_material_idx);

	load_model("sponza.obj", "sponza_crytek/");
	load_model("2b.obj",     "yorha_2b/");
	//load_model("cube.obj", "/");
}
