#include <filesystem>
#include <rendercat/scene.hpp>
#include <rendercat/texture_cache.hpp>
#include <rendercat/util/color_temperature.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <fmt/core.h>
#include <imguizmo/ImGuizmo.h>
#include <zcm/geometric.hpp>
#include <zcm/type_ptr.hpp>
#include <zcm/quaternion.hpp>
#include <glbinding-aux/Meta.h>
#include <tracy/Tracy.hpp>

using namespace rc;


Scene::~Scene()
{
	Material::delete_default_diffuse();
}

void Scene::init()
{
	ZoneScoped;
	{
		ZoneScopedN("load default material");
		Material::set_default_diffuse("assets/materials/missing.tga");
		materials.emplace_back(Material::create_default_material());
	}

	current_cubemap = "assets/cubemaps/field_evening_late";
	//load_skybox_cubemap(current_cubemap);

	main_camera.state.position = {0.0f, 1.7f, 1};
	//main_camera.state.orientation = {0, 0, 1, 0};
	directional_light.direction = zcm::quat::wxyz(0.857, -0.264, 0.281, -0.342);
	directional_light.color_intensity = zcm::vec4{1.00, 0.675, 0.404, 30.0};
	directional_light.ambient_intensity = 10.0f;

	fog.inscattering_environment_opacity = 1.0f;
	fog.dir_inscattering_color = zcm::vec4(1.00, 0.675, 0.404, 1.0f);

	PointLight pl;
	pl.set_position({4.0456166, 1.2134154, 1.4490192});
	pl.set_color({1.0f, 0.2f, 0.1f});
	pl.set_radius(5.0f);
	pl.set_flux(200.0f);

	point_lights.push_back(pl);

	pl.set_position({4.0399876, 1.1741302, -1.5256181});
	point_lights.push_back(pl);

	pl.set_position({-5.1185966, 1.1978881, -1.4758949});
	point_lights.push_back(pl);

	pl.set_position({-5.1236506, 1.1966785, 1.4444633});
	point_lights.push_back(pl);

	pl.set_position({1, 2, 0.6603798});
	pl.set_radius(5);
	pl.set_color({0.9999999, 0.40850836, 0.10151122});
	pl.set_flux(300);
	point_lights.push_back(pl);

	pl.set_position({0.41778356, 1.6590002, -1.1758192});
	pl.set_radius(5);
	pl.set_color({0, 0.22318292, 1});
	pl.set_flux(186);
	point_lights.push_back(pl);

	pl.set_position({-2.695215, 1.9737453, 3.507128});
	pl.set_radius(5.2);
	pl.set_color({0.9999999, 0.6574341, 0.41041234});
	pl.set_flux(272);
	point_lights.push_back(pl);

	pl.set_position({-6.3189254, 2.0678706, -3.7080016});
	pl.set_radius(5);
	pl.set_color({0.9999999, 0.58833313, 0.30886215});
	pl.set_flux(309);
	point_lights.push_back(pl);

	pl.set_position({5.5017176, 6.0245194, 3.4676418});
	pl.set_radius(12.3);
	pl.set_color({0.9999999, 0.48068285, 0.17443053});
	pl.set_flux(3680.999);
	point_lights.push_back(pl);

	pl.set_position({-6.402336, 6.1419916, -3.7815819});
	pl.set_radius(10.5);
	pl.set_color({0.9999999, 0.35502282, 0.05776373});
	pl.set_flux(2109);
	point_lights.push_back(pl);

	SpotLight spot;
	spot.set_position({-10.335639, 2.697124, -1.3429887});
	spot.set_radius(6);
	spot.set_orientation({-0.32663208, 0.108409435, -0.8911168, -0.29575282});
	spot.set_angle_outer(0.62831855);
	spot.set_angle_inner(0.36651915);
	spot.set_color({0.9999999, 0.8278238, 0.71452236});
	spot.set_flux(600);
	spot_lights.push_back(spot);

	spot.set_position({-1.1204133, 0.3240182, 0.97298086});
	spot.set_radius(6);
	spot.set_orientation({0.90255666, 0.20256084, -0.37072715, 0.08319855});
	spot.set_angle_outer(0.6632251);
	spot.set_angle_inner(0.22689281);
	spot.set_color({0.9999999, 0.8404229, 0.7403612});
	spot.set_flux(400);
	spot_lights.push_back(spot);

	spot.set_position({9.3226595, 2.7431679, -1.8163612});
	spot.set_radius(6);
	spot.set_orientation({-0.27721047, 0.07438496, 0.92519474, 0.24826607});
	spot.set_angle_outer(0.57595867);
	spot.set_angle_inner(0.20943952);
	spot.set_color({0.9999999, 0.8815654, 0.82837105});
	spot.set_flux(600);
	spot_lights.push_back(spot);

	spot.set_position({9.359149, 6.8434515, -0.045962736});
	spot.set_radius(6);
	spot.set_orientation({0.6055771, -0.32512623, -0.63993937, -0.34357953});
	spot.set_angle_outer(0.5235988);
	spot.set_angle_inner(0.34906587);
	spot.set_color({0.9, 0.86, 0.88});
	spot.set_flux(256);
	spot_lights.push_back(spot);


	spot.set_position({-9.810236, 7.0966115, 0.04068963});
	spot.set_radius(5.7);
	spot.set_orientation({0.6642561, -0.2798437, 0.63877475, 0.2691062});
	spot.set_angle_outer(0.5061455);
	spot.set_angle_inner(0.36651915);
	spot.set_color({1, 1, 1});
	spot.set_flux(400);
	spot_lights.push_back(spot);

	spot.set_position({1.3185989, 0.53405356, 1.1138073});
	spot.set_radius(5);
	spot.set_orientation({-0.88422865, -0.17345929, -0.42553875, 0.0834769});
	spot.set_angle_outer(0.41887903);
	spot.set_angle_inner(0.296706);
	spot.set_color({1, 0.41377962, 0.99006295});
	spot.set_flux(400);
	spot_lights.push_back(spot);

	spot.set_position({-0.25851816, 2.5280287, -2.0052888});
	spot.set_radius(6.2);
	spot.set_orientation({0.058905452, -0.017316595, -0.9576025, -0.28147426});
	spot.set_angle_outer(0.27925268);
	spot.set_angle_inner(0.19198623);
	spot.set_color({1, 1, 1});
	spot.set_flux(600);
	spot_lights.push_back(spot);

	load_model_gltf("sponza/sponzahr.gltf");
	load_model_gltf("2b_v6/2b_feather.gltf");

	Texture::Cache::clear();
}


Model* Scene::load_model_gltf(const std::filesystem::path& path)
{
	ZoneScoped;
	model::data data;
	auto file = path;
	if (!file.is_absolute())
		file = std::filesystem::path{rc::path::asset::model} / file;

	if(model::load_gltf_file(data, file)) {
		auto base_material_offset = materials.size();

		for(size_t i = 0; i < data.materials.size(); ++i) {
			materials.emplace_back(std::move(data.materials[i]));
		}

		auto& model = models.emplace_back(Model{});
		model.name = file.filename().u8string();
		model.mesh_count = static_cast<uint32_t>(data.primitives.size());

		model.shaded_meshes = std::unique_ptr<uint32_t[]>(new uint32_t[model.mesh_count]);

		for(size_t i = 0; i < data.primitives.size(); ++i) {

			model.shaded_meshes[i] = shaded_meshes.size();
			auto& shaded_mesh = shaded_meshes.emplace_back(ShadedMesh{});
			shaded_mesh.transform.position = data.translate[i];
			shaded_mesh.transform.scale = data.scale[i];
			shaded_mesh.transform.rotation = data.rotation[i];
			shaded_mesh.transform.update();

			if(data.primitive_material[i] < data.materials.size())	{
				shaded_mesh.material = static_cast<uint32_t>(data.primitive_material[i] + base_material_offset);

			} else {
				shaded_mesh.material = missing_material_idx;
			}

			shaded_mesh.mesh = submeshes.size();
			auto& submesh = submeshes.emplace_back(std::move(data.primitives[i]));
			model.bbox.include(submesh.bbox);

		}

		return &model;
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

static bool edit_transform(RigidTransform& transform,
                           const CameraState& camera_state,
                           int *selected_operation,
                           int allowed_modes=ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE,
                           int space = -1)
{
	const auto& main_viewport = ImGui::GetMainViewport();
	ImGuizmo::SetRect(main_viewport->Pos.x, main_viewport->Pos.y,
	                  main_viewport->Size.x, main_viewport->Size.y);

	ImGui::PushID(&transform);
	ImGuizmo::SetID(reinterpret_cast<uintptr_t>(&transform));

	bool changed = false;

	if (allowed_modes & ImGuizmo::OPERATION::TRANSLATE) {
		ImGui::RadioButton("Translate (g)", selected_operation, ImGuizmo::OPERATION::TRANSLATE); ImGui::SameLine();

		if(ImGui::IsKeyPressed(ImGuiKey_G)) {
			*selected_operation = ImGuizmo::OPERATION::TRANSLATE;
		}
	}
	if (allowed_modes & ImGuizmo::OPERATION::ROTATE) {
		ImGui::RadioButton("Rotate (r)", selected_operation, ImGuizmo::OPERATION::ROTATE); ImGui::SameLine();
		if(ImGui::IsKeyPressed(ImGuiKey_R)) {
			*selected_operation = ImGuizmo::OPERATION::ROTATE;
		}
	}
	if (allowed_modes & ImGuizmo::OPERATION::SCALE) {
		ImGui::RadioButton("Scale (s)", selected_operation, ImGuizmo::OPERATION::SCALE);

		if(ImGui::IsKeyPressed(ImGuiKey_S)) {
			*selected_operation = ImGuizmo::OPERATION::SCALE;
		}
	}

	if (space < 0) {
		static bool val = false;
		ImGui::Checkbox("World Space", &val);
		space = val;
	}

	if (allowed_modes & ImGuizmo::OPERATION::TRANSLATE) {
		ImGui::TextUnformatted("Position");
		ImGui::SameLine();
		if (ImGui::Button("Reset##position")) {
			transform.position = zcm::vec3{};
			changed = true;
		}
		changed |= ImGui::DragFloat3("##position",
		                  zcm::value_ptr(transform.position),
		                  0.01f);
		ImGui::Spacing();
	}

	if (allowed_modes & ImGuizmo::OPERATION::ROTATE) {
		ImGui::TextUnformatted("Rotation");
		ImGui::SameLine();
		if (ImGui::Button("Reset##resetrotation")) {
			transform.rotation = zcm::quat{};
			changed = true;
		}

		{
			auto euler = zcm::eulerAngles(transform.rotation);
			auto quat = transform.rotation;
			ImGui::InputFloat3("Euler (pitch-yaw-roll)", zcm::value_ptr(euler), "%.3f",
							   ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat4("Quat (xyzw)", zcm::value_ptr(quat), "%.3f",
							   ImGuiInputTextFlags_ReadOnly);
		}

		ImGui::Spacing();
	}

	if (allowed_modes & ImGuizmo::OPERATION::SCALE) {
		ImGui::TextUnformatted("Scale");
		ImGui::SameLine();
		if (ImGui::Button("Reset##resetscale")) {
			transform.scale = zcm::vec3{1};
			changed = true;
		}
		changed |= ImGui::DragFloat3("##scale", zcm::value_ptr(transform.scale), 0.01f);
	}

	// copy the transform and offset by negative camera position to avoid floating point precision loss.
	auto transform_copy = transform;
	transform_copy.position -= camera_state.position;
	auto transform_mat = transform_copy.get_mat();

	// view matrix without camera translation (already accounted for in the above)
	const auto view = zcm::mat4_cast(camera_state.orientation);
	// usual projection, without reversed-z for simplicity
	const auto proj = make_projection_non_reversed_z(camera_state);

	if (ImGuizmo::Manipulate(zcm::value_ptr(view),
	                         zcm::value_ptr(proj),
	                         static_cast<ImGuizmo::OPERATION>(*selected_operation),
	                         static_cast<ImGuizmo::MODE>(space),
	                         zcm::value_ptr(transform_mat))) {

		zcm::vec3 translate, scale;
		zcm::quat rotate;
		zcm::decompose_orthogonal(transform_mat, scale, rotate, translate);

		switch (*selected_operation) {
		case ImGuizmo::OPERATION::TRANSLATE:
			transform.position = translate + camera_state.position;
			break;
		case ImGuizmo::OPERATION::ROTATE:
			transform.rotation = rotate;
			break;
		case ImGuizmo::OPERATION::SCALE:
			transform.scale = scale;
		}
		changed = true;
	}

	ImGui::PopID();

	if (changed) transform.update();
	return changed;
}

template<typename T>
static bool edit_light(T& light, int id, Camera& camera) noexcept
{
	const auto& main_viewport = ImGui::GetMainViewport();
	ImGuizmo::SetRect(main_viewport->Pos.x, main_viewport->Pos.y,
	                  main_viewport->Size.x, main_viewport->Size.y);
	ImGuizmo::SetID(id | (1 << 29));

	auto& camera_state = camera.state;
	auto view = make_view(camera_state);
	auto proj = make_projection_non_reversed_z(camera_state);

	constexpr bool is_spot = std::is_same_v<T, SpotLight>;
	auto pos = light.position();
	auto diff = light.color();
	float radius = light.radius();

	zcm::quat dir;
	float power;
	float angle_o;
	float angle_i;

	bool light_enabled = light.state & T::Enabled;
	bool light_follow  = light.state & T::FollowCamera;
	bool light_wireframe = light.state & T::ShowWireframe;
	ImGui::Checkbox("Enabled", &light_enabled);
	ImGui::SameLine();
	ImGui::Checkbox("Wireframe", &light_wireframe);
	ImGui::SameLine();
	if (ImGui::Button("Remove?"))
		ImGui::OpenPopup("RemovePopup");

	ImGui::Checkbox("Follow Camera", &light_follow);
	ImGui::SameLine();
	if (ImGui::Button("Light to Camera")) {
		if constexpr(is_spot) {
			dir = camera_state.orientation;
		}
		pos = camera_state.position;
	}
	ImGui::SameLine();
	if (ImGui::Button("Camera To Light")) {
		camera.state.position = pos;
		camera.update();
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy data")) {
		ImGui::OpenPopup("CopyCodePopup");
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::PushItemWidth(-1.0f);

	zcm::mat4 rotate_mat{1.0f};
	static int gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	if constexpr(is_spot) {
		auto spot = static_cast<SpotLight&>(light);
		power = spot.flux();
		dir = spot.orientation();
		angle_o = spot.angle_outer();
		angle_i = spot.angle_inner();

		ImGui::RadioButton("Translate (g)", &gizmo_operation, ImGuizmo::OPERATION::TRANSLATE); ImGui::SameLine();
		ImGui::RadioButton("Rotate (r)", &gizmo_operation, ImGuizmo::OPERATION::ROTATE);

		if(ImGui::IsKeyPressed(ImGuiKey_G)) { // g
			gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
		}

		if(ImGui::IsKeyPressed(ImGuiKey_R)) { // r
			gizmo_operation = ImGuizmo::OPERATION::ROTATE;
		}

		ImGui::TextUnformatted("Direction");
		ImGui::DragFloat4("##lightdirection", zcm::value_ptr(dir), 0.01f);
		rotate_mat = zcm::mat4_cast(dir);
	} else {
		power = light.flux();
	}

	ImGui::TextUnformatted("Position");
	ImGui::DragFloat3("##lightposition", zcm::value_ptr(pos), 0.01f);

	zcm::mat4 transform = zcm::translate(zcm::mat4{1}, pos) * rotate_mat;

	ImGuizmo::Manipulate(zcm::value_ptr(view),
			     zcm::value_ptr(proj),
			     static_cast<ImGuizmo::OPERATION>(gizmo_operation),
			     ImGuizmo::MODE::LOCAL,
			     zcm::value_ptr(transform));

	if (ImGuizmo::IsUsing()) {
		switch (gizmo_operation) {
		case ImGuizmo::OPERATION::TRANSLATE:
			pos.x = transform[3].x;
			pos.y = transform[3].y;
			pos.z = transform[3].z;
			break;
		case ImGuizmo::OPERATION::ROTATE:
			dir = zcm::quat_cast(transform);
			break;
		}

	}

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::TextUnformatted("Diffuse color");
	ImGui::ColorEdit3("##diffusecol",  zcm::value_ptr(diff),
	                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

	ImGui::TextUnformatted("Diffuse color temperature");
	static float color_temp = 6500.0f;
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

	light.set_radius(radius);
	light.set_position(pos);
	light.set_color(diff);

	light.state = T::NoState;
	light.state |= light_follow    ? T::FollowCamera : 0;
	light.state |= light_enabled   ? T::Enabled : 0;
	light.state |= light_wireframe ? T::ShowWireframe : 0;

	if constexpr(is_spot) {
		light.set_orientation(dir);
		light.set_angle_outer(angle_o);
		light.set_angle_inner(angle_i);
		light.set_flux(power);
	} else {
		light.set_flux(power);
	}

	bool ret = true;
	if (ImGui::BeginPopup("RemovePopup")) {
		if(ImGui::Button("Confirm"))
			ret = false;
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopup("CopyCodePopup")) {
		static bool with_pos = true;
		ImGui::Checkbox("Location", &with_pos);
		ImGui::SameLine();
		static bool with_color = true;
		ImGui::Checkbox("Color + power", &with_color);

		static std::string result;
		result.reserve(256);
		result.clear();

		const char* light_name = is_spot ? "spot" : "point";
		if (with_pos) {
			result.append(fmt::format("{}.set_position({{{}, {}, {}}});\n",
			                          light_name,
			                          pos.x, pos.y, pos.z));
			result.append(fmt::format("{}.set_radius({});\n",
			                          light_name,
			                          radius));
			if constexpr(is_spot) {
				result.append(fmt::format("{}.set_orientation({{{}, {}, {}, {}}});\n",
				                          light_name,
				                          dir.w, dir.x, dir.y, dir.z));
				result.append(fmt::format("{}.set_angle_outer({});\n",
				                          light_name,
				                          angle_o));
				result.append(fmt::format("{}.set_angle_inner({});\n",
				                          light_name,
				                          angle_i));
			}
		}

		if (with_color) {
			result.append(fmt::format("{}.set_color({{{}, {}, {}}});\n",
			                          light_name,
			                          diff.x, diff.y, diff.z));
			result.append(fmt::format("{}.set_flux({});\n",
			                          light_name,
			                          power));
		}

		if (ImGui::Button("Copy to clipboard")) {
			ImGui::SetClipboardText(result.c_str());
		}

		ImGui::InputTextMultiline("Code", &result);
		ImGui::EndPopup();
	}
	return ret;
}


static void show_material_ui(rc::Material& material)
{
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
	ImGui::PushStyleColor(ImGuiCol_Text, col);
	if(material.double_sided()) {
		ImGui::TextUnformatted("Double-Sided (backface culling off)");
	} else {
		ImGui::TextUnformatted("Single-Sided (backface culling on)");
	}
	ImGui::PopStyleColor();

	ImGui::TextUnformatted("Texture maps: ");

	auto display_map_info = [](auto& map)
	{
		auto& storage = map.storage();
		if (ImGui::BeginTable("material ui", 2)) {
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 200);
			ImGui::TableSetupColumn("Value");

			if (auto label = storage.label(); !label.empty()) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Label:");
				ImGui::TableNextColumn();
				ImGui::TextWrapped("%s", label.data());

				ImGui::Spacing();
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Dimensions:");
			ImGui::TableNextColumn();
			ImGui::Text("%d x %d (%d mips)",
				    map.width(),
				    map.height(),
				    map.levels());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Color space:");

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(Texture::enum_value_str(storage.color_space()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Channels:");
			ImGui::TableNextColumn();
			ImGui::Text("%d", storage.channels());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Internal format:");
			ImGui::TableNextColumn();
			ImGui::Text("%s", Texture::enum_value_str(storage.format()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Min Filter:");
			ImGui::TableNextColumn();


			const std::array<Texture::MinFilter, 6> min_filters = {
				Texture::MinFilter::Nearest,
				Texture::MinFilter::Linear,
				Texture::MinFilter::LinearMipMapNearest,
				Texture::MinFilter::LinearMipMapLinear,
				Texture::MinFilter::NearestMipMapNearest,
				Texture::MinFilter::NearestMipMapLinear,
			};

			ImGui::PushItemWidth(-1.0f);
			if (ImGui::BeginCombo("##minfiltercombo", Texture::enum_value_str(map.min_filter()))) {
				for (auto& filter : min_filters) {

					bool is_selected = (filter == map.min_filter());
					auto filter_str = Texture::enum_value_str(filter);

					if (ImGui::Selectable(filter_str, is_selected))
						map.set_filtering(filter, map.mag_filter());
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Mag Filter:");
			ImGui::TableNextColumn();
			const std::array<Texture::MagFilter, 2> mag_filters = {
				Texture::MagFilter::Nearest,
				Texture::MagFilter::Linear
			};

			ImGui::PushItemWidth(-1.0f);
			if (ImGui::BeginCombo("##magfiltercombo", Texture::enum_value_str(map.mag_filter()))) {
				for (auto& filter : mag_filters) {

					bool is_selected = (filter == map.mag_filter());
					auto filter_str = Texture::enum_value_str(filter);

					if (ImGui::Selectable(filter_str, is_selected))
						map.set_filtering(map.min_filter(), filter);
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Aniso Samples:");
			ImGui::TableNextColumn();
			float aniso = map.anisotropy();
			ImGui::PushItemWidth(-1.0f);
			if (ImGui::SliderFloat("##anisosamples", &aniso, 1.0f, 16.0f)) {
				map.set_anisotropy(aniso);
			}
			ImGui::PopItemWidth();


			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Swizzle Mask:");
			ImGui::TableNextColumn();
			float width = ImGui::GetContentRegionAvail().x - 5.0f;

			auto print_swizzle_channel = [](auto& component, float width) {

				auto get_color = [](auto component){
					ImVec4 col;
					switch (component) {
					case Texture::ChannelValue::Red:
						col = {1.0f, 0.0f, 0.0f, 1.0f};
						break;
					case Texture::ChannelValue::Green:
						col = {0.0f, 1.0f, 0.0f, 1.0f};
						break;
					case Texture::ChannelValue::Blue:
						col = {0.0f, 0.0f, 1.0f, 1.0f};
						break;
					case Texture::ChannelValue::Alpha:
						col = {1.0f, 0.0f, 1.0f, 1.0f};
						break;
					case Texture::ChannelValue::Zero:
						col = {0.5f, 0.5f, 0.5f, 1.0f};
						break;
					case Texture::ChannelValue::One:
						col = {1.0f, 1.0f, 1.0f, 1.0f};
						break;
					};
					return col;
				};

				const std::array<Texture::ChannelValue, 6> channels {
					Texture::ChannelValue::Red,
					Texture::ChannelValue::Green,
					Texture::ChannelValue::Blue,
					Texture::ChannelValue::Alpha,
					Texture::ChannelValue::Zero,
					Texture::ChannelValue::One
				};

				bool changed = false;
				ImGui::PushItemWidth(width);
				ImGui::PushID(&component);
				ImGui::PushStyleColor(ImGuiCol_Text, get_color(component));
				if (ImGui::BeginCombo("##swizzlecombo", Texture::enum_value_str(component))) {
					for (auto& channel : channels) {

						bool is_selected = (channel == component);
						auto channel_str = Texture::enum_value_str(channel);

						ImGui::PushStyleColor(ImGuiCol_Text, get_color(channel));
						if (ImGui::Selectable(channel_str, is_selected)) {
							component = channel;
							changed = true;
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();
						ImGui::PopStyleColor();
					}
					ImGui::EndCombo();
				}
				ImGui::PopStyleColor();
				ImGui::PopID();
				ImGui::PopItemWidth();

				return changed;
			};

			auto mask = map.swizzle_mask();

			bool changed = false;
			changed |= print_swizzle_channel(mask.red, width * 0.25f - 10.0f); ImGui::SameLine();
			changed |= print_swizzle_channel(mask.green, width * 0.25f - 10.0f); ImGui::SameLine();
			changed |= print_swizzle_channel(mask.blue, width * 0.25f - 10.0f); ImGui::SameLine();
			changed |= print_swizzle_channel(mask.alpha, width * 0.25f - 10.0f); ImGui::SameLine();
			if (changed) {
				map.set_swizzle_mask(mask);
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("GL Handle:");
			ImGui::TableNextColumn();
			ImGui::Text("%d", storage.texture_handle());

			if(storage.shared()) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Shared:");
				ImGui::TableNextColumn();
				ImGui::Text("%s  (refcount: %d)", storage.ref_count() > 1 ? "True" : "False", storage.ref_count());
			}
			ImGui::EndTable();
		}

		if(!storage.valid())
			return;

		if (map.levels() > 1) {
			float bias = map.mip_bias();
			if (ImGui::SliderFloat("MIP bias", &bias, 0, map.levels()-1)) {
				map.set_mip_bias(bias);
			}
		}

		// NOTE: imgui assumes direct3d convention, custom UVs for OpenGL are below
		const auto uv0 = ImVec2(0.0f, 1.0f);
		const auto uv1 = ImVec2(1.0f, 0.0f);
		const auto width = ImGui::GetContentRegionAvail().x - 5.0f;
		const auto storage_width = static_cast<float>(storage.width());
		const auto storage_height = static_cast<float>(storage.height());
		const auto height = width * (storage_height / storage_width);
		const auto handle = static_cast<uintptr_t>(storage.texture_handle());
		ImGui::Image(reinterpret_cast<ImTextureID>(handle), ImVec2(width, height), uv0, uv1);
	};

	auto display_map = [&material,&display_map_info](auto& map, auto kind)
	{
		ImGui::PushID(Texture::enum_value_str(kind));
		if(material.has_texture_kind(kind) && ImGui::TreeNode("##mapparams", "%s Map", Texture::enum_value_str(kind))) {
			display_map_info(map);
			ImGui::TreePop();
		}
		ImGui::PopID();
	};

	float mat_bias = material.textures.base_color_map.mip_bias();
	auto max_levels = std::max(material.textures.base_color_map.levels(), material.textures.occlusion_roughness_metallic_map.levels());
	max_levels = std::max(max_levels, material.textures.occlusion_map.levels());
	max_levels = std::max(max_levels, material.textures.normal_map.levels());
	max_levels = std::max(max_levels, material.textures.emission_map.levels());
	if (ImGui::SliderFloat("Material MIP bias", &mat_bias, 0.0f, max_levels-1)) {
		auto set_bias = [](auto& map, float bias) {
			if (map.valid()) {
				map.set_mip_bias(bias);
			}
		};
		set_bias(material.textures.base_color_map, mat_bias);
		set_bias(material.textures.normal_map, mat_bias);
		set_bias(material.textures.occlusion_roughness_metallic_map, mat_bias);
		set_bias(material.textures.occlusion_map, mat_bias);
		set_bias(material.textures.emission_map, mat_bias);
	}

	display_map(material.textures.base_color_map,  Texture::Kind::BaseColor);
	display_map(material.textures.normal_map,   Texture::Kind::Normal);
	display_map(material.textures.occlusion_roughness_metallic_map, Texture::Kind::RoughnessMetallic);
	display_map(material.textures.occlusion_roughness_metallic_map, Texture::Kind::OcclusionRoughnessMetallic);
	display_map(material.textures.occlusion_map, Texture::Kind::OcclusionSeparate);
	display_map(material.textures.emission_map, Texture::Kind::Emission);
	ImGui::Spacing();

	auto material_data = material.data();
	if (material_data) {
		if (ImGui::Button("Unmap")) {
			material.unmap();
			return;
		}

		ImGui::TextUnformatted("Diffuse color");
		bool changed = false;
		changed |= ImGui::ColorEdit4("##matdiffcolor", zcm::value_ptr(material_data->base_color_factor),
			ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

		ImGui::TextUnformatted("Emissive color");
		changed |= ImGui::ColorEdit3("##matemissivecolor", zcm::value_ptr(material_data->emissive_factor),
			ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

		if (material.has_texture_kind(Texture::Kind::Normal)) {
			changed |= ImGui::SliderFloat("Normal Scale", &(material_data->normal_scale), -10.0f, 10.0f);
		}
		changed |= ImGui::SliderFloat("Roughness", &(material_data->roughness_factor), 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Metallic",  &(material_data->metallic_factor),  0.0f, 1.0f);
		if (material.has_texture_kind(Texture::Kind::Occlusion)) {
			changed |= ImGui::SliderFloat("Occlusion Strength", &(material_data->occlusion_strength), 0.0f, 1.0f);
		}

		ImGui::TextUnformatted("Flags:");

		bool has_alpha_mask = material.alpha_mode() == Texture::AlphaMode::Mask;
		if (ImGui::Checkbox("Alpha Mask", &has_alpha_mask)) {
			if (has_alpha_mask)
				material.set_alpha_mode(Texture::AlphaMode::Mask);
			else
				material.set_alpha_mode(Texture::AlphaMode::Opaque);
			changed = true;
		}

		bool has_normal_without_z = material_data->type & (RC_SHADER_TEXTURE_NORMAL_WITHOUT_Z);
		if (ImGui::Checkbox("Normal map without Z component", &has_normal_without_z)) {
			if (has_normal_without_z)
				material_data->type |= (RC_SHADER_TEXTURE_NORMAL_WITHOUT_Z);
			else
				material_data->type &= ~(RC_SHADER_TEXTURE_NORMAL_WITHOUT_Z);
			changed = true;
		}

		if (changed)
			material.flush();
	} else {
		if (ImGui::Button("Map")) {
			material.map();
		}
	}
}

static void show_mesh_ui(rc::model::Mesh& submesh, rc::Material& material)
{

	if (ImGui::BeginTable("mesh_ui", 2)) {
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 200);
		ImGui::TableSetupColumn("Value");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Vertices / unique:");
		ImGui::TableNextColumn();
		ImGui::Text("%u / %u (%u%%)",
			    submesh.numverts,
			    submesh.numverts_unique,
			    rc::math::percent(submesh.numverts_unique, submesh.numverts));

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Index Range:");
		ImGui::TableNextColumn();
		if (submesh.index_min > submesh.index_max) {
			ImGui::TextUnformatted("Unset");
		} else {
			ImGui::Text("%u - %u", submesh.index_min, submesh.index_max);
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		auto index_type = glbinding::aux::Meta::getString(static_cast<gl::GLenum>(submesh.index_type));
		ImGui::TextUnformatted("Index Type:");
		ImGui::TableNextColumn();
		ImGui::Text("%s", index_type.data());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		auto draw_mode = glbinding::aux::Meta::getString(static_cast<gl::GLenum>(submesh.draw_mode));
		ImGui::TextUnformatted("Draw Mode:");
		ImGui::TableNextColumn();
		ImGui::Text("%s", draw_mode.data());


		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Tangents:");
		ImGui::TableNextColumn();
		ImGui::Text("%s", (submesh.has_tangents ? "Baked" : "Shader"));
		ImGui::EndTable();
	}

	auto bbox_min = submesh.bbox.min();
	auto bbox_max = submesh.bbox.max();
	ImGui::TextUnformatted("BBox Min");
	ImGui::InputFloat3("##bboxmin", zcm::value_ptr(bbox_min), "%.3f", ImGuiInputTextFlags_ReadOnly);
	ImGui::TextUnformatted("BBox Max");
	ImGui::InputFloat3("##bboxmax", zcm::value_ptr(bbox_max), "%.3f", ImGuiInputTextFlags_ReadOnly);

	if (ImGui::TreeNodeEx("##submeshmaterial",
	                      ImGuiTreeNodeFlags_NoTreePushOnOpen,
	                      "Submesh Material: %s", material.name.data()))
	{
		show_material_ui(material);
	}
}

void Scene::update()
{
	for(auto& pl : point_lights) {
		if(pl.state & PointLight::FollowCamera) {
			pl.set_position(main_camera.state.position);
		}
	}
	for(auto& sl : spot_lights) {
		if(sl.state & PointLight::FollowCamera) {
			sl.set_orientation(zcm::conjugate(main_camera.state.orientation));
			sl.set_position(main_camera.state.position);
		}
	}

	if (!window_shown) {
		return;
	}

	if(!ImGui::Begin("Scene", &window_shown)) {
		ImGui::End();
		return;
	}


	if(ImGui::CollapsingHeader("Camera")) {
		ImGui::InputFloat3("Position", zcm::value_ptr(main_camera.state.position));
		ImGui::SliderFloat4("Orientation", zcm::value_ptr(main_camera.state.orientation), -1.f, 1.f);
		if (ImGui::Button("Pos1")) {
			main_camera.state.position = {-2.266f, 1.7f, -0.566f};
			main_camera.state.orientation = zcm::quat::wxyz(0.603f, 0.054f, 0.793f, 0.071f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Pos2")) {
			main_camera.state.position = {8.5f, 1.7f, 0.0f};
			main_camera.state.orientation = zcm::quat::wxyz(0.705f, 0.006f, -0.709f, -0.006f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Pos3")) {
			main_camera.state.position = {-7.905f, 5.647f, 1.588f};
			main_camera.state.orientation = zcm::quat::wxyz(0.854f, 0.179f, 0.478f, 0.1f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Pos4")) {
			main_camera.state.position = {10.f, 20.f, 30.f};
			main_camera.state.orientation = zcm::quat::wxyz(-0.213f, 0.031f, 0.967f, -0.139f);
		}

		if (ImGui::TreeNode("Alternate Orientation")) {
			auto forward = -main_camera.state.get_backward();
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
			ImGui::TreePop();
		}

		ImGui::Spacing();

		float camera_fov_deg = zcm::degrees(main_camera.state.fov);
		if (ImGui::SliderFloat("FOV", &camera_fov_deg, FPSCameraBehavior::fov_min, FPSCameraBehavior::fov_max))
		{
			main_camera.state.fov = zcm::radians(camera_fov_deg);
		}
		ImGui::SliderFloat("Near Z", &main_camera.state.znear, CameraState::znear_min, 10.0f);
		//ImGui::SliderFloat("Far Z", &main_camera.state.zfar, main_camera.state.znear, 1000.0f);
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
				main_camera.state.fov = zcm::mix(fov_a, fov_b, t);
				main_camera.state.exposure = zcm::mix(exp_a, exp_b, t);

				if (ImGui::Button("Reset Start State")) {
					has_a = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset Start State")) {
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
			ImGui::PushItemWidth(-1.0f);
			ImGui::TextUnformatted("Directional inscattering color");
			ImGui::ColorEdit4("##directionalinscatteringcolor", zcm::value_ptr(fog.dir_inscattering_color),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

			if(ImGui::Button("Use Sun Diffuse")) {
				fog.dir_inscattering_color = zcm::vec4(directional_light.color_intensity.rgb, fog.dir_inscattering_color.w);
			}
			ImGui::Spacing();
			ImGui::Spacing();

			ImGui::TextUnformatted("Directional Exponent");
			ImGui::SliderFloat("##directionalexponent", &fog.dir_exponent, 1.0f, 64.0f);
			ImGui::TextUnformatted("Inscattering opacity");
			ImGui::SliderFloat("##inscatteringopacity", &fog.inscattering_environment_opacity, 0.0f, 1.0f);
			ImGui::TextUnformatted("Inscattering density");
			ImGui::SliderFloat("##inscatteringdensity", &fog.inscattering_density, 0.0f, 1.0f);
			ImGui::TextUnformatted("Extinction density");
			ImGui::SliderFloat("##extinctiondensity", &fog.extinction_density, 0.0f, 1.0f);

			fog.state = ExponentialDirectionalFog::NoState;
			fog.state |= enabled ? ExponentialDirectionalFog::Enabled : 0;

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Environment")) {
			skyboxes_list();
			ImGui::TreePop();
		}

		if(ImGui::TreeNode("Sunlight")) {

			ImGui::PushItemWidth(-1.0f);
			RigidTransform transform;
			transform.rotation = directional_light.direction;
			transform.position = main_camera.state.position - 2 * main_camera.state.get_backward();
			int operation = ImGuizmo::OPERATION::ROTATE;
			if (edit_transform(transform, main_camera.state, &operation, ImGuizmo::OPERATION::ROTATE, ImGuizmo::MODE::LOCAL)) {
				directional_light.direction = transform.rotation;
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextUnformatted("Color");

			zcm::vec3 color = directional_light.color_intensity.rgb;
			zcm::vec2 intensity = {directional_light.color_intensity.a, directional_light.ambient_intensity};


			if (ImGui::ColorEdit3("##directional_color",   zcm::value_ptr(color),
			                      ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel))
			{
				directional_light.color_intensity = zcm::vec4{color, intensity.r};
			}

			if (ImGui::SliderFloat2("Intensity", zcm::value_ptr(intensity), 0.0f, 1e3f))
			{
				directional_light.color_intensity = zcm::vec4{color, intensity.x};
				directional_light.ambient_intensity = intensity.y;
			}

			ImGui::PopItemWidth();
			ImGui::TreePop();
		}

		if(ImGui::TreeNode("Spot Lights")) {
			if(ImGui::Button("Add light")) {
				SpotLight sp;
				sp.set_orientation(zcm::conjugate(main_camera.state.orientation));
				sp.set_position(main_camera.state.position);
				spot_lights.push_back(sp);
			}
			for(unsigned i = 0; i < spot_lights.size(); ++i) {
				ImGui::PushID(i);
				if(ImGui::TreeNode("SpotLight", "Spot light #%d", i)) {
					if(!edit_light<SpotLight>(spot_lights[i], i, main_camera)) {
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
				pl.set_position(main_camera.state.position);
				point_lights.push_back(pl);
			}
			for(unsigned i = 0; i < point_lights.size(); ++i) {
				ImGui::PushID(i);
				if(ImGui::TreeNode("PointLight", "Point light #%d", i)) {
					auto& light = point_lights[i];
					if(!edit_light<PointLight>(light, i, main_camera)) {
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
				show_material_ui(materials[i]);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

	}

	static int model_transform_mode;

	if(ImGui::CollapsingHeader("Models")) {
		for(unsigned i = 0; i < models.size(); ++i) {
			Model& model = models[i];
			ImGui::PushID(i);
			if(ImGui::TreeNode("Model", "%s", model.name.data())) {


				edit_transform(model.transform, main_camera.state, &model_transform_mode);

				if(ImGui::TreeNodeEx("##submeshes", ImGuiTreeNodeFlags_CollapsingHeader, "Submeshes: %u", model.mesh_count)) {
					for(unsigned j = 0; j < model.mesh_count; ++j) {
						auto& shaded_mesh = shaded_meshes[model.shaded_meshes[j]];

						auto submesh_idx = shaded_mesh.mesh;
						auto material_idx = shaded_mesh.material;

						auto& submesh = submeshes[submesh_idx];

						ImGui::PushID(submesh.name.begin().operator->(), submesh.name.end().operator->());
						if (ImGui::TreeNode("##submesh", "%s", submesh.name.data())) {
							edit_transform(shaded_mesh.transform, main_camera.state, &model_transform_mode);
							show_mesh_ui(submesh, materials[material_idx]);
							ImGui::TreePop();
						}
						ImGui::PopID();
					}
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
	ImGui::End();
}

void Scene::load_skybox_equirectangular(std::string_view name)
{
	ZoneScoped;
	cubemap.load_equirectangular(name);
	cubemap_diffuse_irradiance = Cubemap::integrate_diffuse_irradiance(cubemap);
	cubemap_specular_environment = Cubemap::convolve_specular(cubemap);
}

void Scene::load_skybox_cubemap(std::string_view path)
{
	ZoneScoped;
	cubemap.load_cube(path);
	cubemap_diffuse_irradiance = Cubemap::integrate_diffuse_irradiance(cubemap);
	cubemap_specular_environment = Cubemap::convolve_specular(cubemap);
}

void Scene::skyboxes_list()
{
	static std::filesystem::path current = current_cubemap;

	ImGui::PushItemWidth(-1.0f);
	if (ImGui::BeginCombo("##skyboxeslist", current_cubemap.c_str())) {
		static std::filesystem::path skyboxes_dir = rc::path::asset::cubemap;
		for (auto&& entry : std::filesystem::directory_iterator{skyboxes_dir, std::filesystem::directory_options::skip_permission_denied}) {
			if (entry.is_regular_file() || entry.is_directory())  {
				auto path = entry.path();

				if (path.extension() == ".hdr" || entry.is_directory()) {

					bool is_selected = (path == current);
					auto path_str = path.u8string();

					if (ImGui::Selectable(path_str.c_str(), is_selected))
						current = path;
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
			}
		}
		if (current_cubemap != current) {
			current_cubemap = current.u8string();
			if (current.has_extension())
				load_skybox_equirectangular(current_cubemap);
			else
				load_skybox_cubemap(current_cubemap);
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
}


