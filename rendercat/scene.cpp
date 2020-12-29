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

using namespace rc;


void Scene::init()
{
	{
		Material::set_default_diffuse("assets/materials/missing.tga");
		materials.emplace_back(Material::create_default_material());
	}

	current_cubemap = "assets/cubemaps/courtyard.hdr";
	load_skybox_equirectangular(current_cubemap);

	main_camera.state.position = {0.0f, 1.4f, 1};
	//main_camera.state.orientation = {0, 0, 1, 0};
	directional_light.direction = zcm::normalize(directional_light.direction);

	PointLight pl;
	pl.set_position({4.0f, 1.0f, 1.0f});
	pl.set_color({1.0f, 0.2f, 0.1f});
	pl.set_radius(4.5f);
	pl.set_flux(175.0f);

	point_lights.push_back(pl);

	pl.set_position({4.0f, 1.0f, -1.5f});
	point_lights.push_back(pl);

	pl.set_position({-5.0f, 1.0f, -1.5f});
	point_lights.push_back(pl);

	pl.set_position({-5.0f, 1.0f, 1.0f});
	point_lights.push_back(pl);

	pl.set_position({1.0f, 1.0f, 1.0f});
	point_lights.push_back(pl);

	pl.set_position({1.0f, 1.5f, -1.0f});
	pl.set_color({0.1f, 0.2f, 1.0f});
	point_lights.push_back(pl);

	SpotLight sp;
	sp.set_color({0.9f, 0.86f, 0.88f});
	sp.set_orientation({0.0f, 0.0f, 0.714f, 0.7f});
	sp.set_radius(6.0f);
	sp.set_flux(160.0f);

	sp.set_position({-10.0f, 3.3f, -3.8f});
	spot_lights.push_back(sp);

	sp.set_position({-10.0f, 3.3f, 3.4f});
	spot_lights.push_back(sp);

	sp.set_position({9.3f, 3.3f, -3.8f});
	spot_lights.push_back(sp);

	sp.set_position({9.3f, 3.3f, 3.4f});
	spot_lights.push_back(sp);

	load_model_gltf("sponzahr.gltf", "sponza/");
	load_model_gltf("2b_feather.gltf", "2b_v6/");

	Texture::Cache::clear();
}


Model* Scene::load_model_gltf(const std::string_view name, const std::string_view basedir)
{
	model::data data;
	if(model::load_gltf_file(data, name, basedir)) {
		auto base_material_offset = materials.size();

		for(size_t i = 0; i < data.materials.size(); ++i) {
			materials.emplace_back(std::move(data.materials[i]));
		}

		auto& model = models.emplace_back(Model{});
		model.name = name.substr(0, name.find_last_of('.'));
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

static bool edit_transform(RigidTransform& transform, int id, const CameraState& camera_state)
{
	const auto& main_viewport = ImGui::GetPlatformIO().MainViewport;
	ImGuizmo::SetRect(main_viewport->Pos.x, main_viewport->Pos.y,
	                  main_viewport->Size.x, main_viewport->Size.y);

	ImGui::PushID(&transform);
	ImGuizmo::SetID(id);

	bool changed = false;

	static int gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	ImGui::RadioButton("Translate (g)", &gizmo_operation, ImGuizmo::OPERATION::TRANSLATE); ImGui::SameLine();
	ImGui::RadioButton("Rotate (r)", &gizmo_operation, ImGuizmo::OPERATION::ROTATE); ImGui::SameLine();
	ImGui::RadioButton("Scale (s)", &gizmo_operation, ImGuizmo::OPERATION::SCALE);

	static bool gizmo_space = false;
	ImGui::Checkbox("World Space", &gizmo_space);

	if(ImGui::IsKeyPressed(71)) { // g
		gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
	}

	if(ImGui::IsKeyPressed(82)) { // r
		gizmo_operation = ImGuizmo::OPERATION::ROTATE;
	}

	if(ImGui::IsKeyPressed(83)) { // s
		gizmo_operation = ImGuizmo::OPERATION::SCALE;
	}

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
		ImGui::InputFloat3("Quat (xyzw)", zcm::value_ptr(quat), "%.3f",
		                   ImGuiInputTextFlags_ReadOnly);
	}


	ImGui::Spacing();
	ImGui::TextUnformatted("Scale");
	ImGui::SameLine();
	if (ImGui::Button("Reset##resetscale")) {
		transform.scale = zcm::vec3{1};
		changed = true;
	}
	changed |= ImGui::DragFloat3("##scale", zcm::value_ptr(transform.scale), 0.01f);

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
	                         static_cast<ImGuizmo::OPERATION>(gizmo_operation),
	                         gizmo_space ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL,
	                         zcm::value_ptr(transform_mat))) {

		zcm::vec3 translate, scale;
		zcm::quat rotate;
		zcm::decompose_orthogonal(transform_mat, scale, rotate, translate);

		switch (gizmo_operation) {
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
	const auto& main_viewport = ImGui::GetPlatformIO().MainViewport;
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

		if(ImGui::IsKeyPressed(71)) { // g
			gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
		}

		if(ImGui::IsKeyPressed(82)) { // r
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

	auto display_map_info = [](const auto& map)
	{
		auto& storage = map.storage();
		ImGui::Columns(2, "material_ui", false);

		ImGui::TextUnformatted("Dimensions:"); ImGui::NextColumn();
		ImGui::Text("%d x %d (%d mips)",
			    map.width(),
			    map.height(),
			    map.levels()); ImGui::NextColumn();

		ImGui::TextUnformatted("Color space:"); ImGui::NextColumn();
		ImGui::TextUnformatted(Texture::enum_value_str(storage.color_space())); ImGui::NextColumn();

		ImGui::TextUnformatted("Channels:"); ImGui::NextColumn();
		ImGui::Text("%d", storage.channels()); ImGui::NextColumn();

		ImGui::TextUnformatted("Internal format:"); ImGui::NextColumn();
		ImGui::Text("%s", Texture::enum_value_str(storage.format())); ImGui::NextColumn();

		ImGui::TextUnformatted("Min Filter:"); ImGui::NextColumn();
		ImGui::Text("%s", Texture::enum_value_str(map.min_filter())); ImGui::NextColumn();

		ImGui::TextUnformatted("Mag Filter:"); ImGui::NextColumn();
		ImGui::Text("%s", Texture::enum_value_str(map.mag_filter())); ImGui::NextColumn();

		ImGui::TextUnformatted("Aniso Samples:"); ImGui::NextColumn();
		ImGui::Text("%d", map.anisotropy()); ImGui::NextColumn();

		ImGui::TextUnformatted("Swizzle Mask:"); ImGui::NextColumn();

		auto print_swizzle_channel = [](auto component) {

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

			ImGui::TextColored(col, "%s", Texture::enum_value_str(component));
		};

		print_swizzle_channel(map.swizzle_mask().red); ImGui::SameLine();
		print_swizzle_channel(map.swizzle_mask().green); ImGui::SameLine();
		print_swizzle_channel(map.swizzle_mask().blue); ImGui::SameLine();
		print_swizzle_channel(map.swizzle_mask().alpha); ImGui::SameLine();
		ImGui::NextColumn();

		ImGui::TextUnformatted("GL Handle:"); ImGui::NextColumn();
		ImGui::Text("%d", storage.texture_handle()); ImGui::NextColumn();

		if(storage.shared()) {
			ImGui::TextUnformatted("Shared:"); ImGui::NextColumn();
			ImGui::Text("%d", storage.ref_count()); ImGui::NextColumn();
		}
		ImGui::Columns(1);

		if(!storage.valid())
			return;

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

	auto display_map = [&material,&display_map_info](const auto& map, auto kind)
	{
		ImGui::PushID(Texture::enum_value_str(kind));
		if(material.has_texture_kind(kind) && ImGui::TreeNode("##mapparams", "%s Map", Texture::enum_value_str(kind))) {
			display_map_info(map);
			ImGui::TreePop();
		}
		ImGui::PopID();
	};

	display_map(material.textures.base_color_map,  Texture::Kind::BaseColor);
	display_map(material.textures.normal_map,   Texture::Kind::Normal);
	display_map(material.textures.occlusion_roughness_metallic_map, Texture::Kind::RoughnessMetallic);
	display_map(material.textures.occlusion_roughness_metallic_map, Texture::Kind::OcclusionRoughnessMetallic);
	display_map(material.textures.occlusion_map, Texture::Kind::OcclusionSeparate);
	display_map(material.textures.emission_map, Texture::Kind::Emission);
	ImGui::Spacing();

	if (material.data) {

		if (ImGui::Button("Unmap")) {
			ImGui::OpenPopup("RemovePopup");
		}

		ImGui::TextUnformatted("Diffuse color");
		bool changed = false;
		changed |= ImGui::ColorEdit4("##matdiffcolor", zcm::value_ptr(material.data->base_color_factor),
			ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

		ImGui::TextUnformatted("Emissive color");
		changed |= ImGui::ColorEdit3("##matemissivecolor", zcm::value_ptr(material.data->emissive_factor),
			ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);

		if (material.has_texture_kind(Texture::Kind::Normal)) {
			changed |= ImGui::SliderFloat("Normal Scale", &(material.data->normal_scale), -10.0f, 10.0f);
		}
		changed |= ImGui::SliderFloat("Roughness", &(material.data->roughness_factor), 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Metallic",  &(material.data->metallic_factor),  0.0f, 1.0f);
		if (material.has_texture_kind(Texture::Kind::Occlusion)) {
			changed |= ImGui::SliderFloat("Occlusion Strength", &(material.data->occlusion_strength), 0.0f, 1.0f);
		}
		if (changed)
			material.flush();
	} else {
		if (ImGui::Button("Map")) {
			material.map();
		}
	}

	if (ImGui::BeginPopup("RemovePopup")) {
		if(ImGui::Button("Confirm")) {
			material.unmap();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

}

static void show_mesh_ui(rc::model::Mesh& submesh, rc::Material& material)
{

	ImGui::Columns(2, "mesh_ui", false);

	ImGui::TextUnformatted("Vertices / unique:"); ImGui::NextColumn();
	ImGui::Text("%u / %u (%u%%)",
	            submesh.numverts,
	            submesh.numverts_unique,
	            rc::math::percent(submesh.numverts_unique, submesh.numverts)); ImGui::NextColumn();

	ImGui::TextUnformatted("Touched lights:"); ImGui::NextColumn();
	ImGui::Text("%u", submesh.touched_lights); ImGui::NextColumn();

	ImGui::TextUnformatted("Index Range:"); ImGui::NextColumn();
	ImGui::Text("%u - %u", submesh.index_min, submesh.index_max); ImGui::NextColumn();

	auto index_type = glbinding::aux::Meta::getString(static_cast<gl::GLenum>(submesh.index_type));
	ImGui::TextUnformatted("Index Type:"); ImGui::NextColumn();
	ImGui::Text("%s", index_type.data()); ImGui::NextColumn();

	auto draw_mode = glbinding::aux::Meta::getString(static_cast<gl::GLenum>(submesh.draw_mode));
	ImGui::TextUnformatted("Draw Mode:"); ImGui::NextColumn();
	ImGui::Text("%s", draw_mode.data()); ImGui::NextColumn();

	ImGui::TextUnformatted("Tangents:"); ImGui::NextColumn();
	ImGui::Text("%s", (submesh.has_tangents ? "Baked" : "Shader")); ImGui::NextColumn();
	ImGui::Columns(1);

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

	if(!ImGui::Begin("Scene", &window_shown)) {
		ImGui::End();
		return;
	}


	if(ImGui::CollapsingHeader("Camera")) {
		ImGui::InputFloat3("Position", zcm::value_ptr(main_camera.state.position));
		ImGui::SliderFloat4("Orientation", zcm::value_ptr(main_camera.state.orientation), -1.f, 1.f);

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
			ImGui::TextUnformatted("Inscattering color");
			ImGui::PushItemWidth(-1.0f);
			ImGui::ColorEdit4("##inscatteringcolor", zcm::value_ptr(fog.inscattering_color),
			                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel);
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

		if(ImGui::TreeNode("Directional")) {

			ImGui::PushItemWidth(-1.0f);
			ImGui::TextUnformatted("Direction");
			ImGui::SliderFloat3("##direction", zcm::value_ptr(directional_light.direction), -1.0, 1.0);
			directional_light.direction = zcm::normalize(directional_light.direction);
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TextUnformatted("Color");

			zcm::vec3 color = directional_light.color_intensity.rgb;
			float intensity = directional_light.color_intensity.a;

			if (ImGui::ColorEdit3("##directional_color",   zcm::value_ptr(color),
			                      ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoLabel))
			{
				directional_light.color_intensity = zcm::vec4{color, intensity};
			}

			if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1e3f))
			{
				directional_light.color_intensity = zcm::vec4{color, intensity};
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

	if(ImGui::CollapsingHeader("Models")) {
		for(unsigned i = 0; i < models.size(); ++i) {
			Model& model = models[i];
			ImGui::PushID(i);
			if(ImGui::TreeNode("Model", "%s", model.name.data())) {

				edit_transform(model.transform, i, main_camera.state);

				if(ImGui::TreeNodeEx("##submeshes", ImGuiTreeNodeFlags_CollapsingHeader, "Submeshes: %u", model.mesh_count)) {
					for(unsigned j = 0; j < model.mesh_count; ++j) {
						auto& shaded_mesh = shaded_meshes[model.shaded_meshes[j]];

						auto submesh_idx = shaded_mesh.mesh;
						auto material_idx = shaded_mesh.material;

						auto& submesh = submeshes[submesh_idx];

						ImGui::PushID(submesh.name.begin().operator->(), submesh.name.end().operator->());
						if (ImGui::TreeNode("##submesh", "%s", submesh.name.data())) {
							edit_transform(shaded_mesh.transform, i * model.mesh_count + j, main_camera.state);
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
	cubemap.load_equirectangular(name);
	cubemap_diffuse_irradiance = cubemap.integrate_diffuse_irradiance();
	cubemap_specular_environment = cubemap.convolve_specular();
}

void Scene::load_skybox_cubemap(std::string_view path)
{
	cubemap.load_cube(path);
	cubemap_diffuse_irradiance = cubemap.integrate_diffuse_irradiance();
	cubemap_specular_environment = cubemap.convolve_specular();
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


