#include <scene.hpp>
#include <tiny_obj_loader.h>

#include <iostream>
#include <unordered_map>
#include <imgui.h>


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

	std::string material_path{"assets/materials/models/"};
	material_path.append(basedir);

	std::cerr << "\n--- loading model \'" << name << "\' from \'" << model_path << "\' ------------------------\n";
	size_t vertex_cout = 0, unique_vertex_count = 0;


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
		Material material(mat.name);
		auto spec_color = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
		material.specularColorShininess(spec_color, mat.shininess);
		std::cerr << " * material:\t\'" << mat.name << "\'\tformat_id: " << i << "\n";
		std::cerr << "   ~ material params:\tspecular:  " << spec_color
		          << ",  shininess:  " << mat.shininess
		          << ",  dissolve:  " << mat.dissolve << '\n';


		if(!mat.diffuse_texname.empty()) {
			std::cerr << "   ~ loading diffuse \'" << mat.diffuse_texname << '\'';
			material.addDiffuseMap(mat.diffuse_texname, material_path);
			std::cerr << '\n';
		} else {
			std::cerr << "   - MISSING diffuse map\n";
		}

		if(!mat.normal_texname.empty()) {
			std::cerr << "   ~ loading normal \'" << mat.normal_texname << '\'';
			material.addNormalMap(mat.normal_texname, material_path);
			std::cerr << '\n';
		}

		if(!mat.specular_texname.empty()) {
			std::cerr << "   ~ loading specular \'" << mat.specular_texname << '\'';
			material.addSpecularMap(mat.specular_texname, material_path);
			std::cerr << '\n';
		}

		materials.emplace_back(std::move(material));
		std::cerr << "   + ready\tformat_id:  " << i << ",  scene_id:  " << materials.size()-1 << '\n' << std::endl;
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
			const auto mat_name = mat.name;

			auto pos = std::find_if(materials.begin(), materials.end(), [&mat_name](const auto& mat)
			{
				return mat.m_name == mat_name;
			});
			if(pos != materials.end()) {
				instance.material_id = std::distance(materials.begin(), pos);
				//std::cerr << "   ~ material \'" << mat.name << "\' (" << mat_id << ',' << pos->second  << ")\n";
			} else {
				std::cerr << "   - could not find " << mat_name  << " in materials!\n";
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

		vertex_cout += indices.size();
		unique_vertex_count += vertices.size();

		//std::cerr << "   + done loading submesh,  vertices:  " << indices.size() << "  unique:  " << vertices.size() << '\n' << std::endl;
		meshes.emplace_back(Mesh(std::move(vertices), std::move(indices)));
		instance.mesh_id = meshes.size()-1;
		instances.emplace_back(std::move(instance));
	}
	std::cerr << " ~ final vertex count: " << vertex_cout << ", unique: " << unique_vertex_count << " (" << 100-m::percent(unique_vertex_count, vertex_cout) << "% saved)";
	std::cerr << "\n--- model \'" << name <<"\' loaded -------------------------------\n" << std::endl;
}

Scene::Scene()
{
	Material::set_default_diffuse();
	Material::set_texture_cache(&m_texture_cache);

	cubemap.load_textures({
		"left.hdr",
		"right.hdr",
		"top.hdr",
		"bottom.hdr",
		"front.hdr",
		"back.hdr"
		}, "assets/materials/cubemaps/evening_field/");

	main_camera.pos = {0.0f, 2.0f, 2.0f};
	PointLight pl;
	pl.position({8.0f, 2.0f, 2.0f})
	  .ambient({0.0f, 0.0f, 0.0f})
	  .diffuse({1.0, 0.2, 0.1})
	  .specular({0.3, 0.05, 0.0})
	  .radius(10.0)
	  .flux(150.0);

	lights.push_back(pl);

	pl.position({8.0f, 2.0f, -3.0f});
	lights.push_back(pl);

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

	materials.emplace_back(Material{"missing"});

	load_model("sponza.obj", "sponza_crytek/");
	load_model("2b.obj",     "yorha_2b/");
}

void Scene::update()
{
	ImGui::Begin("Scene", &window_shown, glm::vec2(300.0, 100.0));
	if(ImGui::CollapsingHeader("Camera")) {
		ImGui::InputFloat3("Position", glm::value_ptr(main_camera.pos));
		ImGui::SliderFloat("FOV", &main_camera.fov, 30.0f, 120.0f);
		ImGui::SliderFloat("Near Z", &main_camera.znear, 0.001f, 10.0f);
		ImGui::SliderFloat("Exposure", &main_camera.exposure, 0.001f, 10.0f);
	}

	if(ImGui::CollapsingHeader("Lighting")) {
		if(ImGui::TreeNode("Directional")) {

			ImGui::SliderFloat3("Direction", glm::value_ptr(directional_light.direction), -1.0, 1.0);
			ImGui::ColorEdit3("Ambient",   glm::value_ptr(directional_light.ambient),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
			ImGui::ColorEdit3("Diffuse",   glm::value_ptr(directional_light.diffuse),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
			ImGui::ColorEdit3("Specular",  glm::value_ptr(directional_light.specular),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
			ImGui::TreePop();
		}

		if(ImGui::TreeNode("Point Lights")) {
			if(ImGui::Button("Add light")) {
				lights.push_back(PointLight{});
			}
			for(unsigned i = 0; i < lights.size(); ++i) {
				ImGui::PushID(i);
				if(ImGui::TreeNode("PointLight", "Point light #%d", i)) {
					auto& light = lights[i];

					auto pos = light.position();
					auto amb = light.ambient();
					auto diff = light.diffuse();
					auto spec = light.specular();
					float power = light.flux();
					float radius = light.radius();

					ImGui::Checkbox("Enabled", &light.enabled);
					ImGui::SameLine();
					if (ImGui::Button("Remove?"))
						ImGui::OpenPopup("RemovePopup");

					ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f);
					ImGui::ColorEdit3("Ambient",  glm::value_ptr(amb),
					                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
					ImGui::ColorEdit3("Diffuse",  glm::value_ptr(diff),
					                    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
					ImGui::ColorEdit3("Specular", glm::value_ptr(spec),
					                    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
					ImGui::DragFloat("Flux (lm)", &power,  1.0f, 1.0f, 100000.0f);
					ImGui::DragFloat("Radius",    &radius, 0.1f, 0.5f, 1000.0f);

					light
					  .radius(radius)
					  .flux(power)
					  .position(pos)
					  .ambient(amb)
					  .diffuse(diff)
					  .specular(spec);

					if (ImGui::BeginPopup("RemovePopup")) {
						if(ImGui::Button("Confirm"))
							lights.erase(std::next(lights.begin(), i));
						ImGui::EndPopup();
					}

					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		} // TreeNode("Point Lights")
	} // CollapsingHeader("Lighting")


	if(ImGui::CollapsingHeader("Materials")) {
		for(unsigned i = 0; i < materials.size(); ++i) {
			ImGui::PushID(i);
			if(ImGui::TreeNode("Material", "%s", materials[i].m_name.data())) {
				ImGui::ColorEdit3("Specular Color", glm::value_ptr(materials[i].m_specular_color),
					ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
				ImGui::DragFloat("Shininess", &(materials[i].m_shininess), 0.1f, 0.01f, 128.0f);

				int texture_count = 1;
				if(materials[i].type() & Material::SpecularMapped)
					++texture_count;
				if(materials[i].type() & Material::NormalMapped)
					++texture_count;


				auto width = ImGui::GetContentRegionAvailWidth() / texture_count - 5.0f;
				if(materials[i].m_diffuse_map)
					ImGui::Image((ImTextureID)(uintptr_t)materials[i].m_diffuse_map, ImVec2(width,width));

				if(materials[i].m_normal_map) {
					ImGui::SameLine();
					ImGui::Image((ImTextureID)(uintptr_t)materials[i].m_normal_map, ImVec2(width,width));
				}
				if(materials[i].m_specular_map) {
					ImGui::SameLine();
					ImGui::Image((ImTextureID)(uintptr_t)materials[i].m_specular_map, ImVec2(width,width));
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

	}
	ImGui::End();
}
