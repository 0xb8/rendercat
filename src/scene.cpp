#include <rendercat/scene.hpp>
#include <rendercat/util/color_temperature.hpp>
#include <imgui.hpp>

Scene::Scene()
{
	Material::set_default_diffuse();
	materials.emplace_back(Material{"missing"});
	Material::set_texture_cache(&m_texture_cache);

	cubemap.load_textures("assets/materials/cubemaps/field_evening");

	main_camera.pos = {0.0f, 1.5f, 0.4f};
	directional_light.direction = glm::normalize(directional_light.direction);
	PointLight pl;
	pl.position({4.0f, 1.0f, 1.0f})
	  .ambient({0.0f, 0.0f, 0.0f})
	  .diffuse({1.0f, 0.2f, 0.1f})
	  .specular({0.3f, 0.05f, 0.0f})
	  .radius(3.5f)
	  .flux(75.0f);

	point_lights.push_back(pl);

	pl.position({4.0f, 1.0f, -1.5f});
	point_lights.push_back(pl);

	pl.position({-5.0f, 1.0f, -1.5f});
	point_lights.push_back(pl);

	pl.position({-5.0f, 1.0f, 1.0f});
	point_lights.push_back(pl);

	pl.position({1.0f, 1.0f, 1.0f});
	point_lights.push_back(pl);

	pl.position({1.0f, 1.5f, -1.0f})
	  .diffuse({0.1, 0.2, 1.0})
	  .specular({0.0, 0.05, 0.3});
	point_lights.push_back(pl);

	SpotLight sp;
	sp.diffuse({0.9f, 0.86f, 0.88f})
	  .specular({0.4f, 0.395f, 0.395f})
	  .direction({0.0f, 1.0f, 0.0f})
	  .radius(4.0f)
	  .flux(60.0f);

	sp.position({-10.0f, 3.3f, -3.8f});
	spot_lights.push_back(sp);

	sp.position({-10.0f, 3.3f, 3.4f});
	spot_lights.push_back(sp);

	sp.position({9.3f, 3.3f, -3.8f});
	spot_lights.push_back(sp);

	sp.position({9.3f, 3.3f, 3.4f});
	spot_lights.push_back(sp);

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

template<typename T>
static bool edit_light(PunctualLight<T>& pl) noexcept
{
	constexpr bool is_spot = std::is_same_v<T, SpotLight>;
	auto pos = pl.position();
	auto amb = pl.ambient();
	auto diff = pl.diffuse();
	auto spec = pl.specular();
	float radius = pl.radius();
	float color_temp = pl.color_temperature();

	glm::vec3 dir;
	float power;
	float angle_o;
	float angle_i;

	bool light_enabled = pl.state & PunctualLight<T>::Enabled;
	bool light_follow  = pl.state & PunctualLight<T>::FollowCamera;
	bool light_wireframe = pl.state & PunctualLight<T>::ShowWireframe;
	ImGui::Checkbox("Enabled", &light_enabled);
	ImGui::SameLine();
	ImGui::Checkbox("Wireframe", &light_wireframe);
	ImGui::SameLine();
	if (ImGui::Button("Remove?"))
		ImGui::OpenPopup("RemovePopup");

	ImGui::Checkbox("Follow Camera", &light_follow);
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::PushItemWidth(-1.0f);

	if constexpr(is_spot) {
		auto spot = static_cast<SpotLight&>(pl);
		power = spot.flux();
		dir = spot.direction();
		angle_o = spot.angle_outer();
		angle_i = spot.angle_inner();
		ImGui::TextUnformatted("Direction");
		ImGui::DragFloat3("##lightdirection", glm::value_ptr(dir), 0.01f);
	} else {
		power = pl.flux();
	}

	ImGui::TextUnformatted("Position");
	ImGui::DragFloat3("##lightposition", glm::value_ptr(pos), 0.01f);

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::TextUnformatted("Ambient color");
	ImGui::ColorEdit3("##ambientcol",  glm::value_ptr(amb),
	                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
	ImGui::TextUnformatted("Diffuse color");
	ImGui::ColorEdit3("##diffusecol",  glm::value_ptr(diff),
	                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

	ImGui::TextUnformatted("Diffuse color temperature");
	if(ImGui::SliderFloat("##diffusetemp", &color_temp, 273.0f, 10000.0f, "%.0f K")) {
		diff = rc::util::temperature_to_linear_color(color_temp);
	}

	ImGui::TextUnformatted("Specular color");
	ImGui::PushItemWidth(-50.0f);
	ImGui::ColorEdit3("##specularcol", glm::value_ptr(spec),
	                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
	ImGui::SameLine();
	if(ImGui::Button("Use Diff")) {
		spec = diff;
	}

	ImGui::PopItemWidth();
	ImGui::PopItemWidth();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::DragFloat("Flux (lm)", &power,  1.0f, 1.0f, 100000.0f);
	show_flux_help();
	ImGui::DragFloat("Radius",    &radius, 0.1f, 0.5f, 1000.0f);

	if constexpr(is_spot) {
		ImGui::Spacing();
		ImGui::SliderAngle("Angle Outer",  &angle_o, 0.0f, 89.0f);
		ImGui::SliderAngle("Angle Inner",  &angle_i, 0.0f, 89.0f); // TODO: add slider for softness
	}

	pl.radius(radius)
	  .position(pos)
	  .ambient(amb)
	  .diffuse(diff)
	  .specular(spec)
	  .color_temperature(color_temp);

	pl.state = PunctualLight<T>::NoState;
	pl.state |= light_follow    ? PunctualLight<T>::FollowCamera : 0;
	pl.state |= light_enabled   ? PunctualLight<T>::Enabled : 0;
	pl.state |= light_wireframe ? PunctualLight<T>::ShowWireframe : 0;

	if constexpr(is_spot) {
		static_cast<SpotLight&>(pl).direction(dir)
		                           .angle_outer(angle_o)
			                   .angle_inner(angle_i)
			                   .flux(power);
	} else {
		pl.flux(power);
	}



	bool ret = true;
	if (ImGui::BeginPopup("RemovePopup")) {
		if(ImGui::Button("Confirm"))
			ret = false;
		ImGui::EndPopup();
	}
	return ret;
}

void Scene::update()
{
	for(auto& pl : point_lights) {
		if(pl.state & PointLight::FollowCamera) {
			pl.position(main_camera.pos);
		}
	}
	for(auto& sl : spot_lights) {
		if(sl.state & PointLight::FollowCamera) {
			sl.direction(-main_camera.front).position(main_camera.pos);
		}
	}

	if(!ImGui::Begin("Scene", &window_shown)) {
		ImGui::End();
		return;
	}

	if(ImGui::CollapsingHeader("Renderer")) {
		const char* labels[] = {"Disabled", "2x", "4x", "8x", "16x"};

		ImGui::Combo("MSAA", &desired_msaa_level, labels, 5);
		show_help_tooltip("Multi-Sample Antialiasing\n\nValues > 4x may be unsupported on some setups.");
		ImGui::SliderFloat("Resolution scale", &desired_render_scale, 0.1f, 1.0f, "%.1f");
		ImGui::Checkbox("Show submesh AABB", &draw_aabb);
	}

	if(ImGui::CollapsingHeader("Camera")) {
		ImGui::InputFloat3("Position", glm::value_ptr(main_camera.pos));
		ImGui::SliderFloat("FOV", &main_camera.fov, 30.0f, 120.0f);
		ImGui::SliderFloat("Near Z", &main_camera.znear, 0.001f, 10.0f);
		ImGui::SliderFloat("Far Z", &main_camera.zfar, main_camera.znear, 1000.0f);
		ImGui::SliderFloat("Exposure", &main_camera.exposure, 0.001f, 10.0f);

		bool frustum_locked = main_camera.frustum.state & rc::Frustum::Locked;
		bool frustum_debug = main_camera.frustum.state & rc::Frustum::ShowWireframe;
		ImGui::Checkbox("Frustum Locked", &frustum_locked);
		ImGui::SameLine();
		ImGui::Checkbox("Show Wireframe", &frustum_debug);
		main_camera.frustum.state = 0;
		main_camera.frustum.state |= frustum_locked ? rc::Frustum::Locked : 0;
		main_camera.frustum.state |= frustum_debug ? rc::Frustum::ShowWireframe : 0;
	}

	if(ImGui::CollapsingHeader("Lighting")) {
		if(ImGui::TreeNode("Fog")) {
			bool enabled = fog.state & ExponentialDirectionalFog::Enabled;
			ImGui::Checkbox("Enabled", &enabled);
			ImGui::TextUnformatted("Inscattering color");
			ImGui::PushItemWidth(-1.0f);
			ImGui::ColorEdit4("##inscatteringcolor", glm::value_ptr(fog.inscattering_color),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
			ImGui::TextUnformatted("Directional inscattering color");
			ImGui::ColorEdit4("##directionalinscatteringcolor", glm::value_ptr(fog.dir_inscattering_color),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

			if(ImGui::Button("Use Sun Diffuse")) {
				fog.dir_inscattering_color = glm::vec4(directional_light.diffuse, fog.dir_inscattering_color.w);
			}
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::TextUnformatted("Directional Exponent");
			ImGui::SliderFloat("##directionalexponent", &fog.dir_exponent, 1.0f, 64.0f);
			ImGui::TextUnformatted("Inscattering density");
			ImGui::SliderFloat("##inscatteringdensity", &fog.inscattering_density, 0.0f, 1.0f, "%.3f", 2.0f);
			ImGui::TextUnformatted("Extinction density");
			ImGui::SliderFloat("##extinctiondensity", &fog.extinction_density, 0.0f, 1.0f, "%.3f", 2.0f);

			fog.state = ExponentialDirectionalFog::NoState;
			fog.state |= enabled ? ExponentialDirectionalFog::Enabled : 0;

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}

		if(ImGui::TreeNode("Directional")) {

			ImGui::PushItemWidth(-1.0f);
			ImGui::TextUnformatted("Direction");
			ImGui::SliderFloat3("##direction", glm::value_ptr(directional_light.direction), -1.0, 1.0);
			directional_light.direction = glm::normalize(directional_light.direction);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextUnformatted("Ambient");
			ImGui::ColorEdit3("##ambientcolor",   glm::value_ptr(directional_light.ambient),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
			ImGui::TextUnformatted("Diffuse");
			ImGui::ColorEdit3("##diffusecolor",   glm::value_ptr(directional_light.diffuse),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
			ImGui::TextUnformatted("Specular");
			ImGui::ColorEdit3("##specularcolor",  glm::value_ptr(directional_light.specular),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

			ImGui::PopItemWidth();
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
					if(!edit_light<SpotLight>(spot_lights[i])) {
						spot_lights.erase(std::next(spot_lights.begin(), i));
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
					if(!edit_light<PointLight>(light)) {
						point_lights.erase(std::next(point_lights.begin(), i));
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

				auto col = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
				ImGui::PushStyleColor(ImGuiCol_Text, col);
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
					ImGui::TextUnformatted("No face culling ");

				ImGui::PopStyleColor();


				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::TextUnformatted("Specular color");
				ImGui::ColorEdit3("##matspeccolor", glm::value_ptr(materials[i].m_specular_color),
					ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
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
				ImGui::PushItemWidth(-1.0f);
				ImGui::TextUnformatted("Position (XYZ)");
				ImGui::DragFloat3("##modelpos", glm::value_ptr(model.position), 0.01f);
				glm::vec3 rot = glm::degrees(model.rotation_euler);
				ImGui::TextUnformatted("Rotation (Yaw Pitch Roll)");
				ImGui::SliderFloat3("##modelrot", glm::value_ptr(rot), -180.0f, 180.0f, "%.1f\u00B0");
				rot = glm::radians(rot);
				model.rotation_euler = rot;
				model.update_transform();

				if(ImGui::TreeNode("Submeshes", "Submeshes: %u", model.submesh_count)) {
					ImGui::Columns(3);
					ImGui::SetColumnOffset(2, ImGui::GetColumnOffset(3) - 65.0f);
					ImGui::SetColumnOffset(1, ImGui::GetColumnOffset(2) - 110.0f);
					ImGui::Text("Name");      ImGui::NextColumn();
					ImGui::Text("Vertices");  ImGui::NextColumn();
					ImGui::Text("Lights");    ImGui::NextColumn();
					ImGui::Separator();
					for(unsigned j = 0; j < model.submesh_count; ++j) {
						const auto& submesh = submeshes[model.submeshes[j]];
						ImGui::Text("%s", submesh.name.data());	ImGui::NextColumn();
						ImGui::Text("%u / %u", submesh.numverts, submesh.numverts_unique); ImGui::NextColumn();
						ImGui::Text("%u", submesh.touched_lights); ImGui::NextColumn();
					}
					ImGui::Columns();
					ImGui::TreePop();
				}

				ImGui::PopItemWidth();
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
}

