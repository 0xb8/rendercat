#include <scene.hpp>

#include <stb_image.h>
#include <iostream>
#include <unordered_map>
#include <tiny_obj_loader.h>

static const float max_aniso = 16.0f;




size_t Scene::load_model(std::string_view path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::string err;


	if(!tinyobj::LoadObj(&attrib, &shapes, nullptr,&err, path.data()))
		throw std::runtime_error(err);

	const bool has_normals = !attrib.normals.empty();
	const bool has_texcoords = !attrib.texcoords.empty();
	if(!has_normals) {
		std::cerr << "Model " << path << " does not have vertex normals!" << std::endl;
	}
	if(!has_texcoords) {
		std::cerr << "Model " << path << " does not have texture coords!" << std::endl;
	}


	std::vector<Mesh::vertex> vertices;
	std::vector<uint32_t> indices;

	std::unordered_map<Mesh::vertex, uint32_t> uniqueVertices;

	for(const tinyobj::shape_t& shape : shapes) {
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

			if (uniqueVertices.count(vert) == 0) {
		            uniqueVertices[vert] = static_cast<uint32_t>(vertices.size());
		            vertices.push_back(vert);
		        }

		        indices.push_back(uniqueVertices[vert]);
		}
	}

	std::cerr << "Loaded model " << path << " with " << vertices.size() << " unique vertices" << std::endl;

	meshes.emplace_back(Mesh(std::move(vertices), indices));
	return meshes.size()-1;
}

size_t Scene::load_material(std::string_view diffuse, std::string_view normal)
{
	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	Material mat;

	int width, height, nrChannels;
	unsigned char *data = stbi_load(diffuse.data(), &width, &height, &nrChannels, 0);
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		mat.diffuse = tex;
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::cout << "Failed to load diffuse map:" << diffuse << std::endl;
	}
	stbi_image_free(data);

	data = stbi_load(normal.data(), &width, &height, &nrChannels, 0);
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		mat.normal = tex;
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::cout << "Failed to load normal map: " << normal << std::endl;
	}
	stbi_image_free(data);

	if(mat.diffuse) {
		materials.push_back(mat);
		return materials.size()-1;
	}
	return -1;
}





Scene::Scene()
{
	stbi_set_flip_vertically_on_load(1);
	Instance ins;
	ins.material_id = load_material("assets/materials/marble.jpg", "assets/materials/marble_normal.jpg");
	ins.mesh_id = load_model("assets/models/cyborg.obj");
	instances.push_back(ins);

	ins.material_id = load_material("assets/materials/sheet.jpg", "");
	ins.mesh_id = load_model("assets/models/cube.obj");
	ins.position = {0.0f, 0.0f, -1.5f};
	ins.axis = {0.25f, 0.4f, 0.2f};
	ins.angle = glm::radians(33.3f);
	instances.push_back(ins);




}


