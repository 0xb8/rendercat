#include <rendercat/scene.hpp>
#include <rendercat/util/color_temperature.hpp>
#include <imgui.hpp>
#include <zcm/geometric.hpp>
#include <zcm/type_ptr.hpp>
#include <zcm/quaternion.hpp>

using namespace rc;

Scene::Scene()
{
	{
		Material::set_default_diffuse("assets/materials/missing.tga");
		materials.emplace_back(Material::create_default_material());
		Material::set_texture_cache(&m_texture_cache);
	}

	cubemap.load_textures("assets/materials/cubemaps/field_evening");

	main_camera.behavior.target = {0.0f, 1.5f, 0.4f};
	directional_light.direction = zcm::normalize(directional_light.direction);
	PointLight pl;
	pl.position = {4.0f, 1.0f, 1.0f};
	pl.diffuse = {1.0f, 0.2f, 0.1f};
	pl.radius = 3.5f;
	pl.set_flux(75.0f);

	point_lights.push_back(pl);

	pl.position = {4.0f, 1.0f, -1.5f};
	point_lights.push_back(pl);

	pl.position = {-5.0f, 1.0f, -1.5f};
	point_lights.push_back(pl);

	pl.position = {-5.0f, 1.0f, 1.0f};
	point_lights.push_back(pl);

	pl.position ={1.0f, 1.0f, 1.0f};
	point_lights.push_back(pl);

	pl.position = {1.0f, 1.5f, -1.0f};
	pl.diffuse = {0.1, 0.2, 1.0};
	point_lights.push_back(pl);

	SpotLight sp;
	sp.diffuse = {0.9f, 0.86f, 0.88f};
	sp.set_direction({0.0f, 1.0f, 0.0f});
	sp.radius = 4.0f;
	sp.set_flux(60.0f);

	sp.position = {-10.0f, 3.3f, -3.8f};
	spot_lights.push_back(sp);

	sp.position = {-10.0f, 3.3f, 3.4f};
	spot_lights.push_back(sp);

	sp.position = {9.3f, 3.3f, -3.8f};
	spot_lights.push_back(sp);

	sp.position = {9.3f, 3.3f, 3.4f};
	spot_lights.push_back(sp);

	load_model("sponza_scaled.obj", "sponza_crytek/");
	load_model("yorha_2b.obj",      "yorha_2b/");

	m_texture_cache.clear();
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
static bool edit_light(T& pl) noexcept
{
	constexpr bool is_spot = std::is_same_v<T, SpotLight>;
	auto pos = pl.position;
	auto diff = pl.diffuse;
	float radius = pl.radius;
	float color_temp = pl.color_temperature;

	zcm::vec3 dir;
	float power;
	float angle_o;
	float angle_i;

	bool light_enabled = pl.state & T::Enabled;
	bool light_follow  = pl.state & T::FollowCamera;
	bool light_wireframe = pl.state & T::ShowWireframe;
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
		ImGui::DragFloat3("##lightdirection", zcm::value_ptr(dir), 0.01f);
	} else {
		power = pl.flux();
	}

	ImGui::TextUnformatted("Position");
	ImGui::DragFloat3("##lightposition", zcm::value_ptr(pos), 0.01f);

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::TextUnformatted("Diffuse color");
	ImGui::ColorEdit3("##diffusecol",  zcm::value_ptr(diff),
	                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

	ImGui::TextUnformatted("Diffuse color temperature");
	if(ImGui::SliderFloat("##diffusetemp", &color_temp, 273.0f, 10000.0f, "%.0f K")) {
		diff = rc::util::temperature_to_linear_color(color_temp);
	}

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

	pl.radius = radius;
	pl.position = pos;
	pl.diffuse = diff;
	pl.color_temperature = color_temp;

	pl.state = T::NoState;
	pl.state |= light_follow    ? T::FollowCamera : 0;
	pl.state |= light_enabled   ? T::Enabled : 0;
	pl.state |= light_wireframe ? T::ShowWireframe : 0;

	if constexpr(is_spot) {
		pl.set_direction(dir);
		pl.set_angle_outer(angle_o);
                pl.set_angle_inner(angle_i);
                pl.set_flux(power);

	} else {
		pl.set_flux(power);
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
			pl.position = main_camera.state.position;
		}
	}
	for(auto& sl : spot_lights) {
		if(sl.state & PointLight::FollowCamera) {
			sl.set_direction(-main_camera.state.get_forward());
			sl.position = main_camera.state.position;
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
		ImGui::Checkbox("Shadows", &shadows);
		ImGui::Separator();
		ImGui::Checkbox("Show submesh bboxes", &draw_bbox);
	}

	if(ImGui::CollapsingHeader("Camera")) {
		ImGui::InputFloat3("Position", zcm::value_ptr(main_camera.state.position));
		ImGui::SliderFloat4("Orientation", zcm::value_ptr(main_camera.state.orientation), -1.f, 1.f);

		auto forward = main_camera.state.get_forward();
		auto up = main_camera.state.get_up();
		auto right = main_camera.state.get_right();
		ImGui::Spacing();
		ImGui::InputFloat3("Forward", zcm::value_ptr(forward), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Up", zcm::value_ptr(up), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Right", zcm::value_ptr(right), "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::Spacing();

		auto euler_angles = zcm::eulerAngles(main_camera.state.orientation);
		ImGui::InputFloat3("Pitch/Yaw/Roll", zcm::value_ptr(euler_angles), "%.3f", ImGuiInputTextFlags_ReadOnly);
		auto angle = zcm::angle(main_camera.state.orientation);
		auto axis = zcm::axis(main_camera.state.orientation);
		zcm::vec4 angle_axis {angle, axis.x, axis.y, axis.z};
		ImGui::InputFloat4("Angle/Axis", zcm::value_ptr(angle_axis), "%.3f", ImGuiInputTextFlags_ReadOnly);

		ImGui::Spacing();


		float camera_fov_deg = zcm::degrees(main_camera.state.fov);
		if (ImGui::SliderFloat("FOV", &camera_fov_deg, FPSCameraBehavior::fov_min, FPSCameraBehavior::fov_max))
		{
			main_camera.state.fov = zcm::radians(camera_fov_deg);
		}
		ImGui::SliderFloat("Near Z", &main_camera.state.znear, CameraState::znear_min, 10.0f);
		ImGui::SliderFloat("Far Z", &main_camera.state.zfar, main_camera.state.znear, 1000.0f);
		ImGui::SliderFloat("Exposure", &main_camera.state.exposure, 0.001f, 10.0f);

		bool frustum_locked = main_camera.frustum.state & rc::Frustum::Locked;
		bool frustum_debug = main_camera.frustum.state & rc::Frustum::ShowWireframe;
		ImGui::Checkbox("Frustum Locked", &frustum_locked);
		ImGui::SameLine();
		ImGui::Checkbox("Show Wireframe", &frustum_debug);
		main_camera.frustum.state = 0;
		main_camera.frustum.state |= frustum_locked ? rc::Frustum::Locked : 0;
		main_camera.frustum.state |= frustum_debug ? rc::Frustum::ShowWireframe : 0;

		static bool has_a, has_b;

		if (ImGui::TreeNode("Interpolate")) {


			static zcm::quat rot_a, rot_b;
			static zcm::vec3 pos_a, pos_b;
			static float fov_a, fov_b;
			static float exp_a, exp_b;

			if (!has_a && ImGui::Button("Remember Start State")) {
				rot_a = main_camera.state.orientation;
				pos_a = main_camera.state.position;
				fov_a = main_camera.state.fov;
				exp_a = main_camera.state.exposure;
				has_a = true;
			}
			if (!has_b && ImGui::Button("Remember End State")) {
				rot_b = main_camera.state.orientation;
				pos_b = main_camera.state.position;
				fov_b = main_camera.state.fov;
				exp_b = main_camera.state.exposure;
				has_b = true;
			}

			if (has_a && has_b) {
				static float t = 0.0f;
				ImGui::SliderFloat("Interpolate", &t, 0.0f, 1.0f);
				main_camera.state.orientation = zcm::slerp(rot_a, rot_b,t);
				main_camera.state.position = zcm::mix(pos_a, pos_b, t);
				main_camera.state.fov = zcm::radians(zcm::mix(fov_a, fov_b, t));
				main_camera.state.exposure = zcm::mix(exp_a, exp_b, t);

				if (ImGui::Button("Reset")) {
					has_a = false;
					has_b = false;
				}
			}
			ImGui::TreePop();
		} else {
			has_a = false;
			has_b = false;
		}
	}

	if(ImGui::CollapsingHeader("Lighting")) {
		if(ImGui::TreeNode("Fog")) {
			bool enabled = fog.state & ExponentialDirectionalFog::Enabled;
			ImGui::Checkbox("Enabled", &enabled);
			ImGui::TextUnformatted("Inscattering color");
			ImGui::PushItemWidth(-1.0f);
			ImGui::ColorEdit4("##inscatteringcolor", zcm::value_ptr(fog.inscattering_color),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
			ImGui::TextUnformatted("Directional inscattering color");
			ImGui::ColorEdit4("##directionalinscatteringcolor", zcm::value_ptr(fog.dir_inscattering_color),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

			if(ImGui::Button("Use Sun Diffuse")) {
				fog.dir_inscattering_color = zcm::vec4(directional_light.diffuse, fog.dir_inscattering_color.w);
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
			ImGui::SliderFloat3("##direction", zcm::value_ptr(directional_light.direction), -1.0, 1.0);
			directional_light.direction = zcm::normalize(directional_light.direction);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextUnformatted("Ambient");
			ImGui::ColorEdit3("##ambientcolor",   zcm::value_ptr(directional_light.ambient),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
			ImGui::TextUnformatted("Diffuse");
			ImGui::ColorEdit3("##diffusecolor",   zcm::value_ptr(directional_light.diffuse),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
			ImGui::TextUnformatted("Specular");
			ImGui::ColorEdit3("##specularcolor",  zcm::value_ptr(directional_light.specular),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}

		if(ImGui::TreeNode("Spot Lights")) {
			if(ImGui::Button("Add light")) {
				SpotLight sp;
				sp.set_direction(-main_camera.state.get_forward());
				sp.position = main_camera.state.position;
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
				pl.position = main_camera.state.position;
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
				auto& material = materials[i];


				auto col = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
				ImGui::TextUnformatted("Alpha mode:  ");
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, col);

				if(material.alpha_mode() == Texture::AlphaMode::Opaque) {
					ImGui::TextUnformatted("Opaque "); ImGui::SameLine();
				}
				if(material.alpha_mode() == Texture::AlphaMode::Mask) {
					ImGui::TextUnformatted("Alpha Mask "); ImGui::SameLine();
				}
				if(material.alpha_mode() == Texture::AlphaMode::Blend) {
					ImGui::TextUnformatted("Blend "); ImGui::SameLine();
				}
				ImGui::PopStyleColor();
				ImGui::NewLine();
				ImGui::TextUnformatted("Face culling: ");
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				if(material.face_culling_enabled) {
					ImGui::TextUnformatted("Enabled ");
				} else {
					ImGui::TextUnformatted("Disabled ");
				}
				ImGui::PopStyleColor();
				ImGui::TextUnformatted("Texture maps: ");

				auto display_map_info = [](const auto& map)
				{
					auto& storage = map.storage();
					ImGui::Text("Dimensions:      %d x %d (%d mips)",
						    map.width(),
						    map.height(),
						    map.levels());

					ImGui::Text("Color space:     %s", Texture::enum_value_str(storage.color_space()));
					ImGui::Text("Channels:        %d", storage.channels());
					ImGui::Text("Internal format: %s", Texture::enum_value_str(storage.format()));
					ImGui::Text("Mip Mapping:     %s", Texture::enum_value_str(map.mipmapping()));
					ImGui::Text("Filtering:       %s", Texture::enum_value_str(map.filtering()));
					ImGui::Text("Aniso Samples:   %d", map.anisotropy());
					ImGui::Text("Swizzle Mask:    %s %s %s %s",
					            Texture::enum_value_str(map.swizzle_mask().red),
					            Texture::enum_value_str(map.swizzle_mask().green),
					            Texture::enum_value_str(map.swizzle_mask().blue),
					            Texture::enum_value_str(map.swizzle_mask().alpha));
					ImGui::Text("GL Handle:       %d", storage.texture_handle());

					if(storage.shared()) {
						ImGui::Text("Shared: %d", storage.share_count());
					}

					if(!storage.valid())
						return;

					// NOTE: imgui assumes direct3d convention, custom UVs for OpenGL are below
					const auto uv0 = ImVec2(0.0f, 1.0f);
					const auto uv1 = ImVec2(1.0f, 0.0f);
					const auto width = ImGui::GetContentRegionAvailWidth() - 5.0f;
					const auto height = width * ((float)storage.height() / (float)storage.width());
					ImGui::Image((ImTextureID)(uintptr_t)storage.texture_handle(), ImVec2(width, height), uv0, uv1);
				};

				auto display_map = [&material,&display_map_info](const auto& map, auto kind)
				{
					ImGui::PushID(Texture::enum_value_str(kind));
					if(material.has_texture_kind(kind) && ImGui::TreeNode("mapparams", "%s Map", Texture::enum_value_str(kind))) {
						display_map_info(map);
						ImGui::TreePop();
					}
					ImGui::PopID();
				};

				display_map(material.m_diffuse_map,  Texture::Kind::Diffuse);
				display_map(material.m_normal_map,   Texture::Kind::Normal);
				display_map(material.m_specular_map, Texture::Kind::Specular);
				ImGui::Spacing();

				ImGui::TextUnformatted("Diffuse color");
				ImGui::ColorEdit3("##matdiffcolor", zcm::value_ptr(material.m_diffuse_color),
					ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

				ImGui::TextUnformatted("Specular color");
				ImGui::ColorEdit3("##matspeccolor", zcm::value_ptr(material.m_specular_color),
					ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

				ImGui::DragFloat("Shininess",   &(material.m_shininess), 0.1f, 1.0f, 128.0f);
				ImGui::SliderFloat("Roughness", &(material.m_roughness), 0.0f, 1.0f);
				ImGui::SliderFloat("Metallic",  &(material.m_metallic),  0.0f, 1.0f);
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
				ImGui::DragFloat3("##modelpos", zcm::value_ptr(model.position), 0.01f);
				zcm::vec3 rot = zcm::degrees(model.rotation_euler);
				ImGui::TextUnformatted("Rotation (Yaw Pitch Roll)");
				ImGui::SliderFloat3("##modelrot", zcm::value_ptr(rot), -180.0f, 180.0f, "%.1f\u00B0");
				rot = zcm::radians(rot);
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

