#include <rendercat/scene.hpp>
#include <rendercat/util/color_temperature.hpp>
#include <iostream>
#include <imgui.hpp>

Scene::Scene()
{
	Material::set_default_diffuse();
	materials.emplace_back(Material{"missing"});
	Material::set_texture_cache(&m_texture_cache);

	cubemap.load_textures({
		"right.hdr",
		"left.hdr",
		"top.hdr",
		"bottom.hdr",
		"front.hdr",
		"back.hdr"
		}, "assets/materials/cubemaps/evening_field/");

	main_camera.pos = {0.0f, 1.5f, 0.4f};
	PointLight pl;
	pl.position({4.0f, 1.0f, 1.0f})
	  .ambient({0.0f, 0.0f, 0.0f})
	  .diffuse({1.0f, 0.2f, 0.1f})
	  .specular({0.3f, 0.05f, 0.0f})
	  .radius(7.5f)
	  .flux(150.0f);

	point_lights.push_back(pl);

	pl.position({4.0f, 1.0f, -1.5f});
	point_lights.push_back(pl);

	pl.position({-5.0f, 1.0f, -1.5f});;
	point_lights.push_back(pl);

	pl.position({-5.0f, 1.0f, 1.0f});
	point_lights.push_back(pl);

	pl.position({1.0f, 1.0f, 1.0f});
	point_lights.push_back(pl);

	pl.position({1.0f, 1.5f, -1.0f})
	  .diffuse({0.1, 0.2, 1.0})
	  .specular({0.0, 0.05, 0.3});
	point_lights.push_back(pl);

	SpotLight spot;
	spot.direction({0.538f, 0.45f, -0.713f})
	    .angle_outer(glm::radians(30.0f))
	    .angle_inner(glm::radians(25.0f))
	    .position({-9.05f, 2.75f, -3.48f})
	    .ambient({0.0f, 0.0f, 0.0f})
	    .diffuse({0.9f, 0.85f, 0.8f})
	    .specular({0.35f, 0.35f, 0.3f})
	    .radius(10.0f)
	    .flux(150.0f);

	spot_lights.push_back(spot);

	spot.direction({-0.46, 0.29f, 0.84f})
		.position({7.62f, 2.18f, 3.98f});

	spot_lights.push_back(spot);

	load_model("sponza_scaled.obj", "sponza_crytek/");
	load_model("yorha_2b.obj",      "yorha_2b/");
}

Model* Scene::load_model(const std::string_view name, const std::string_view basedir)
{
	model::data data;
	if(model::load_obj_file(&data, name, basedir)) {
		auto base_material = materials.size();

		for(size_t i = 0; i < data.materials.size(); ++i) {
			materials.emplace_back(std::move(data.materials[i]));
		}

		Model model;
		model.name = name.substr(0, name.find_last_of('.'));
		model.submesh_count = data.submeshes.size();
		model.materials = std::unique_ptr<uint32_t[]>(new uint32_t[model.submesh_count]);
		model.submeshes = std::unique_ptr<uint32_t[]>(new uint32_t[model.submesh_count]);

		for(size_t i = 0; i < data.submeshes.size(); ++i) {
			if(data.submesh_material[i] >= 0 && (unsigned)data.submesh_material[i] < data.materials.size())	{
				model.materials[i] =  data.submesh_material[i] + base_material;
			} else {
				model.materials[i] = missing_material_idx;
			}
			submeshes.emplace_back(std::move(data.submeshes[i]));
			model.submeshes[i] = submeshes.size() - 1;
		}
		models.emplace_back(std::move(model));
		return &models.back();
	}
	return nullptr;
}

static void show_help_tooltip(const char* desc)
{
	if(ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(300.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void show_flux_help()
{
	show_help_tooltip("Luminous Power in Lumens");
}

void Scene::update()
{
	if(!ImGui::Begin("Scene", &window_shown)) {
		ImGui::End();
		return;
	}
	if(ImGui::CollapsingHeader("Camera")) {
		ImGui::InputFloat3("Position", glm::value_ptr(main_camera.pos));
		ImGui::SliderFloat("FOV", &main_camera.fov, 30.0f, 120.0f);
		ImGui::SliderFloat("Near Z", &main_camera.znear, 0.001f, 10.0f);
		ImGui::SliderFloat("Exposure", &main_camera.exposure, 0.001f, 10.0f);
	}
	if(ImGui::CollapsingHeader("Renderer")) {
		const char* labels[] = {"Disabled", "2x", "4x", "8x", "16x"};

		ImGui::Combo("MSAA", &desired_msaa_level, labels, 5);
		show_help_tooltip("Multi-Sample Antialiasing\n\nValues > 4x may be unsupported on some setups.");
		ImGui::SliderFloat("Resolution scale", &desired_render_scale, 0.1f, 1.0f, "%.1f");
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

		if(ImGui::TreeNode("Spot Lights")) {
			if(ImGui::Button("Add light")) {
				SpotLight sp;
				sp.direction(-main_camera.front).position(main_camera.pos);
				spot_lights.push_back(sp);
			}
			for(unsigned i = 0; i < spot_lights.size(); ++i) {
				ImGui::PushID(i);
				if(ImGui::TreeNode("SpotLight", "Spot light #%d", i)) {
					auto& light = spot_lights[i];
					auto dir = light.direction();
					auto pos = light.position();
					auto amb = light.ambient();
					auto diff = light.diffuse();
					auto spec = light.specular();
					float power = light.flux();
					float radius = light.radius();
					float angle_o = light.angle_outer();
					float angle_i = light.angle_inner();

					bool flashlight = (i == flashlight_idx);

					ImGui::Checkbox("Enabled", &light.enabled);
					ImGui::SameLine();
					if (ImGui::Button("Remove?"))
						ImGui::OpenPopup("RemovePopup");

					ImGui::Checkbox("Flashlight", &flashlight);

					ImGui::DragFloat3("Direction", glm::value_ptr(dir), 0.01f);
					ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.01f);
					ImGui::ColorEdit3("Ambient",  glm::value_ptr(amb),
					                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
					ImGui::ColorEdit3("Diffuse",  glm::value_ptr(diff),
					                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);

					static float temp;
					if(ImGui::SliderFloat("Color Temperature", &temp, 500.0f, 10000.0f, "%.0f K")) {
						auto col = util::temperature_to_linear_color(temp);
						diff = col;
					}

					ImGui::ColorEdit3("Specular", glm::value_ptr(spec),
					                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
					ImGui::DragFloat("Flux (lm)", &power,  1.0f, 1.0f, 100000.0f);
					show_flux_help();

					ImGui::DragFloat("Radius",    &radius, 0.1f, 0.5f, 1000.0f);
					ImGui::SliderAngle("Angle Outer",  &angle_o, 0.0f, 89.0f);
					ImGui::SliderAngle("Angle Inner",  &angle_i, 0.0f, 89.0f); // TODO: add slider for softness

					if(flashlight) {
						flashlight_idx = i;
						dir = -main_camera.front;
						pos = main_camera.pos;
					} else if(flashlight_idx == i) {
						flashlight_idx = std::numeric_limits<size_t>::max();
					}

					light.direction(dir)
					     .angle_outer(angle_o)
					     .angle_inner(angle_i)
					     .radius(radius)
					     .flux(power)
					     .position(pos)
					     .ambient(amb)
					     .diffuse(diff)
					     .specular(spec);

					if (ImGui::BeginPopup("RemovePopup")) {
						if(ImGui::Button("Confirm"))
							spot_lights.erase(std::next(spot_lights.begin(), i));
						ImGui::EndPopup();
					}

					ImGui::TreePop();
				} // Tree Node (Spot Light #)
				ImGui::PopID();
			} // for each spot light
			ImGui::TreePop();
		} // TreeNode(Spot Lights)

		if(ImGui::TreeNode("Point Lights")) {
			if(ImGui::Button("Add light")) {
				PointLight pl;
				pl.position(main_camera.pos);
				point_lights.push_back(pl);
			}
			for(unsigned i = 0; i < point_lights.size(); ++i) {
				ImGui::PushID(i);
				if(ImGui::TreeNode("PointLight", "Point light #%d", i)) {
					auto& light = point_lights[i];

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

					ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.01f);
					ImGui::ColorEdit3("Ambient",  glm::value_ptr(amb),
					                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
					ImGui::ColorEdit3("Diffuse",  glm::value_ptr(diff),
					                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);

					static float temp;
					if(ImGui::SliderFloat("Color Temperature", &temp, 500.0f, 10000.0f, "%.0f K")) {
						auto col = util::temperature_to_linear_color(temp);
						diff = col;
					}

					ImGui::ColorEdit3("Specular", glm::value_ptr(spec),
					                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
					ImGui::DragFloat("Flux (lm)", &power,  1.0f, 1.0f, 100000.0f);
					show_flux_help();

					ImGui::DragFloat("Radius",    &radius, 0.1f, 0.5f, 1000.0f);

					light.radius(radius)
					     .flux(power)
					     .position(pos)
					     .ambient(amb)
					     .diffuse(diff)
					     .specular(spec);

					if (ImGui::BeginPopup("RemovePopup")) {
						if(ImGui::Button("Confirm"))
							point_lights.erase(std::next(point_lights.begin(), i));
						ImGui::EndPopup();
					}

					ImGui::TreePop();
				} // TreeNode(Point Light #)
				ImGui::PopID();
			} // for each point light
			ImGui::TreePop();
		} // TreeNode(Point Lights)
	} // CollapsingHeader(Lighting)


	if(ImGui::CollapsingHeader("Materials")) {
		for(unsigned i = 0; i < materials.size(); ++i) {
			ImGui::PushID(i);
			if(ImGui::TreeNode("Material", "%s", materials[i].name.data())) {

				auto type = materials[i].flags;
				if(type & Material::Opaque) {
					ImGui::TextUnformatted("Opaque "); ImGui::SameLine();
				}
				if(type & Material::Masked) {
					ImGui::TextUnformatted("Alpha Masked "); ImGui::SameLine();
				}
				if(type & Material::Blended) {
					ImGui::TextUnformatted("Blended "); ImGui::SameLine();
				}
				if(type & Material::NormalMapped) {
					ImGui::TextUnformatted("Normal mapped "); ImGui::SameLine();
				}
				if(type & Material::SpecularMapped) {
					ImGui::TextUnformatted("Specular mapped "); ImGui::SameLine();
				}
				ImGui::NewLine();
				if(type & Material::FaceCullingDisabled)
					ImGui::TextUnformatted("Face Culling Disabled ");


				ImGui::ColorEdit3("Specular Color", glm::value_ptr(materials[i].m_specular_color),
					ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel);
				ImGui::DragFloat("Shininess", &(materials[i].m_shininess), 0.1f, 0.01f, 128.0f);

				int texture_count = 1;
				if(type & Material::SpecularMapped)
					++texture_count;
				if(type & Material::NormalMapped)
					++texture_count;


				// NOTE: imgui assumes direct3d convention, custom UVs for OpenGL are below
				const auto uv0 = ImVec2(0.0f, 1.0f);
				const auto uv1 = ImVec2(1.0f, 0.0f);
				const auto width = ImGui::GetContentRegionAvailWidth() / texture_count - 5.0f;
				if(materials[i].m_diffuse_map)
					ImGui::Image((ImTextureID)(uintptr_t)materials[i].m_diffuse_map, ImVec2(width,width), uv0, uv1);

				if(materials[i].m_normal_map) {
					ImGui::SameLine();
					ImGui::Image((ImTextureID)(uintptr_t)materials[i].m_normal_map, ImVec2(width,width), uv0, uv1);
					show_help_tooltip("This texture looks brighter than it should because it's linear RGB, only displayed here as SRGB.");
				}
				if(materials[i].m_specular_map) {
					ImGui::SameLine();
					ImGui::Image((ImTextureID)(uintptr_t)materials[i].m_specular_map, ImVec2(width,width), uv0, uv1);
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

	}

	if(ImGui::CollapsingHeader("Models")) {
		for(unsigned i = 0; i < models.size(); ++i) {
			Model& model = models[i];
			ImGui::PushID(i);
			if(ImGui::TreeNode("Model", "%s", model.name.data())) {
				ImGui::DragFloat3("Position XYZ", glm::value_ptr(model.position), 0.01f);
				glm::vec3 rot = glm::degrees(model.rotation_euler);
				ImGui::SliderFloat3("Yaw Pitch Roll", glm::value_ptr(rot), -180.0f, 180.0f, "%.1f\u00B0");
				rot = glm::radians(rot);
				model.rotation_euler = rot;
				model.update_transform();

				if(ImGui::TreeNode("Submeshes", "Submeshes: %u", model.submesh_count)) {
					for(unsigned j = 0; j < model.submesh_count; ++j) {

						ImGui::PushID(models.size()+j);
						ImGui::Columns(2);
						ImGui::Text("%s", submeshes[model.submeshes[j]].name.data());
						ImGui::SetColumnOffset(1, ImGui::GetColumnOffset(2) - 180.0f);
						ImGui::NextColumn();
						ImGui::Text("%u verts (%u uniq)",
						            submeshes[model.submeshes[j]].numverts,
						            submeshes[model.submeshes[j]].numverts_unique);
						ImGui::Columns();
						ImGui::PopID();
					}
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
}

