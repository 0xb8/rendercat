#include <rendercat/mesh.hpp>
#include <rendercat/material.hpp>
#include <rendercat/util/gl_debug.hpp>
#include <fx/gltf.h>
#include <fmt/core.h>
#include <numeric>
#include <utility>

#include <zcm/vec3.hpp>
#include <zcm/vec4.hpp>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

#include <tracy/Tracy.hpp>
using namespace gl45core;
#include <tracy/TracyOpenGL.hpp>
using namespace rc;


static Material from_gltf_material(const fx::gltf::Material& mat)
{
	ZoneScoped;
	Material material(mat.name);
	material.set_alpha_mode([](auto mat){
		switch (mat.alphaMode) {
			case fx::gltf::Material::AlphaMode::Opaque:
				return Texture::AlphaMode::Opaque;
			case fx::gltf::Material::AlphaMode::Blend:
				return Texture::AlphaMode::Blend;
			case fx::gltf::Material::AlphaMode::Mask:
				return Texture::AlphaMode::Mask;
		}
		return Texture::AlphaMode::Unknown;
	}(mat));

	material.set_alpha_cutoff(mat.alphaCutoff);
	material.set_base_color_factor({mat.pbrMetallicRoughness.baseColorFactor[0],
	                                mat.pbrMetallicRoughness.baseColorFactor[1],
	                                mat.pbrMetallicRoughness.baseColorFactor[2],
	                                mat.pbrMetallicRoughness.baseColorFactor[3]});

	material.set_emissive_factor({mat.emissiveFactor[0],
	                              mat.emissiveFactor[1],
	                              mat.emissiveFactor[2]});

	material.set_roughness_factor(mat.pbrMetallicRoughness.roughnessFactor);
	material.set_metallic_factor(mat.pbrMetallicRoughness.metallicFactor);
	material.set_double_sided(mat.doubleSided);
	material.flush();

	return material;
}


static std::string get_texture_uri(const fx::gltf::Document& doc, const fx::gltf::Material::Texture& tex)
{
	auto index = tex.index;
	if (index >= 0) {
		const auto& image = doc.images[doc.textures[index].source];
		if (!image.IsEmbeddedResource())
			return image.uri;
	}
	return std::string{};
}


static const fx::gltf::Sampler* get_gltf_sampler(const fx::gltf::Document& doc, const fx::gltf::Material::Texture& tex)
{
	if (tex.index >= 0) {
		auto si = doc.textures[tex.index].sampler;
		if (si >= 0) {
			return &doc.samplers[si];
		}
	}
	return nullptr;
}


static void apply_gltf_sampler(const fx::gltf::Sampler* s, ImageTexture2D& tex) {
	if (!s) return;
	tex.set_wrapping(static_cast<Texture::Wrapping>(s->wrapS), static_cast<Texture::Wrapping>(s->wrapT));

	if (s->minFilter != fx::gltf::Sampler::MinFilter::None && s->magFilter != fx::gltf::Sampler::MagFilter::None) {
		tex.set_filtering(static_cast<Texture::MinFilter>(s->minFilter), static_cast<Texture::MagFilter>(s->magFilter));
	}
}


static Material load_gltf_material(const fx::gltf::Document& doc, int mat_idx, const std::filesystem::path& texture_path)
{
	ZoneScoped;
	fx::gltf::Material mat;
	if (mat_idx >= 0)
		mat = doc.materials.at(mat_idx);

	Material material = from_gltf_material(mat);
	{
		auto diffuse_path = get_texture_uri(doc, mat.pbrMetallicRoughness.baseColorTexture);
		if (!diffuse_path.empty()) {
			auto map = Material::load_image_texture(texture_path / diffuse_path, Texture::ColorSpace::sRGB);
			if (map.valid()) {
				material.set_base_color_map(std::move(map));
				apply_gltf_sampler(get_gltf_sampler(doc, mat.pbrMetallicRoughness.baseColorTexture),
				                   material.textures.base_color_map);
			}
		}
	}
	{
		auto normal_path = get_texture_uri(doc, mat.normalTexture);
		if (!normal_path.empty()) {
			auto map = Material::load_image_texture(texture_path / normal_path, Texture::ColorSpace::Linear);
			if (map.valid()) {
				if (map.channels() < 2) {
					fmt::print(stderr, "Normal map has less than 2 channels: {}\n", normal_path);
				} else {
					material.set_normal_map(std::move(map));
					material.data()->normal_scale = mat.normalTexture.scale;
					apply_gltf_sampler(get_gltf_sampler(doc, mat.normalTexture),
							   material.textures.normal_map);
				}
			}
		}
	}
	{
		auto emission_path = get_texture_uri(doc, mat.emissiveTexture);
		if (!emission_path.empty()) {
			auto map = Material::load_image_texture(texture_path / emission_path, Texture::ColorSpace::Linear);
			if (map.valid()) {
				if (map.channels() < 3) {
					fmt::print(stderr, "Emission map has less than 3 channels: {}\n", emission_path);
				} else {
					material.set_emission_map(std::move(map));
					apply_gltf_sampler(get_gltf_sampler(doc, mat.emissiveTexture),
							   material.textures.emission_map);
				}
			}
		}
	}
	{
		auto roughness_path = get_texture_uri(doc, mat.pbrMetallicRoughness.metallicRoughnessTexture);
		if (!roughness_path.empty()) {
			auto map = Material::load_image_texture(texture_path / roughness_path, Texture::ColorSpace::Linear);
			if (map.valid()) {
				if (map.channels() == 1) {
					std::transform(roughness_path.begin(), roughness_path.end(), roughness_path.begin(), [](auto ch)
					{
						return std::tolower(ch);
					});
					if (roughness_path.find("roughness") != std::string::npos) {
						map.set_swizzle_mask(Texture::SwizzleMask{
						                             Texture::ChannelValue::One,
						                             Texture::ChannelValue::Red,
						                             Texture::ChannelValue::One,
						                             Texture::ChannelValue::One,
						                     });
					} else if (roughness_path.find("metallic") != std::string::npos) {
						map.set_swizzle_mask(Texture::SwizzleMask{
						                             Texture::ChannelValue::One,
						                             Texture::ChannelValue::One,
						                             Texture::ChannelValue::Red,
						                             Texture::ChannelValue::One,
						                     });
					} else if (roughness_path.find("occlusion") != std::string::npos) {
						map.set_swizzle_mask(Texture::SwizzleMask{
						                             Texture::ChannelValue::Red,
						                             Texture::ChannelValue::One,
						                             Texture::ChannelValue::One,
						                             Texture::ChannelValue::One,
						                     });
					} else {
						fmt::print(stderr, "Cannot determine single-channel map kind: {}\n", roughness_path);
					}

				}
				material.textures.occlusion_roughness_metallic_map = std::move(map);
				apply_gltf_sampler(get_gltf_sampler(doc, mat.pbrMetallicRoughness.metallicRoughnessTexture),
				                   material.textures.occlusion_roughness_metallic_map);
				material.set_texture_kind(Texture::Kind::RoughnessMetallic, true);
			}
		}

		auto occlusion_path = get_texture_uri(doc, mat.occlusionTexture);
		if (!occlusion_path.empty()) {
			if (occlusion_path != roughness_path) {
				auto map = Material::load_image_texture(texture_path / occlusion_path, Texture::ColorSpace::Linear);
				if (map.valid()) {
					material.set_occlusion_map(std::move(map));
					apply_gltf_sampler(get_gltf_sampler(doc, mat.occlusionTexture),
					                   material.textures.occlusion_map);
				}
			} else {
				material.set_texture_kind(Texture::Kind::OcclusionSeparate, false);
				material.set_texture_kind(Texture::Kind::Occlusion, true);
			}
			material.data()->occlusion_strength = mat.occlusionTexture.strength;
		}
	}

	material.unmap();

	return material;
}


static uint32_t calc_elemt_type_size(const fx::gltf::Accessor& accessor)
{
	uint32_t elementSize = 0;
	switch (accessor.componentType)
	{
	case fx::gltf::Accessor::ComponentType::Byte:
	case fx::gltf::Accessor::ComponentType::UnsignedByte:
		elementSize = 1;
		break;
	case fx::gltf::Accessor::ComponentType::Short:
	case fx::gltf::Accessor::ComponentType::UnsignedShort:
		elementSize = 2;
		break;
	case fx::gltf::Accessor::ComponentType::Float:
	case fx::gltf::Accessor::ComponentType::UnsignedInt:
		elementSize = 4;
		break;
	case fx::gltf::Accessor::ComponentType::None:
		assert(false);
		unreachable();
	}
	return elementSize;
}


static uint32_t calc_comp_count(const fx::gltf::Accessor& accessor)
{
	switch (accessor.type)
	{
	case fx::gltf::Accessor::Type::Mat2:
		return 4 ;
	case fx::gltf::Accessor::Type::Mat3:
		return 9;
	case fx::gltf::Accessor::Type::Mat4:
		return 16;
	case fx::gltf::Accessor::Type::Scalar:
		return 1;
	case fx::gltf::Accessor::Type::Vec2:
		return 2;
	case fx::gltf::Accessor::Type::Vec3:
		return 3;
	case fx::gltf::Accessor::Type::Vec4:
		return 4;
	case fx::gltf::Accessor::Type::None:
		assert(false);
		unreachable();
	}
	assert(false);
	unreachable();
	return 0;
}


static uint32_t CalculateDataTypeSize(fx::gltf::Accessor const & accessor) noexcept
{
	auto elementSize = calc_elemt_type_size(accessor);
	auto comp_count = calc_comp_count(accessor);
	return elementSize * comp_count;
}


struct rc::model::attr_description_t {
	std::string name;
	std::vector<uint8_t> data;

	uint32_t offset = 0;

	uint32_t elem_count = 0;
	uint32_t elem_byte_size = 0;
	uint32_t comp_type = 0;
	uint32_t comp_count = 0;
	rc::bbox3 bbox;
	uint32_t min_idx = 0xFFFFFFFF;
	uint32_t max_idx = 0;
};


static rc::model::attr_description_t gltf_attr_data(int accessor_idx, const fx::gltf::Document& doc)
{
	const auto& accessor = doc.accessors.at(accessor_idx);

	rc::bbox3 _bbox;
	if (accessor.min.size() == 3 && accessor.max.size() == 3) {
		 _bbox.include({accessor.min[0], accessor.min[1], accessor.min[2]});
		 _bbox.include({accessor.max[0], accessor.max[1], accessor.max[2]});
	}

	const auto& buffer_view = doc.bufferViews.at(accessor.bufferView);
	const auto& buffer = doc.buffers.at(buffer_view.buffer);

	auto dtype_size = CalculateDataTypeSize(accessor);

	rc::model::attr_description_t res;
	res.bbox = _bbox;

	uint32_t buffer_stride = buffer_view.byteStride ? buffer_view.byteStride : dtype_size;

	res.data.reserve(accessor.count * dtype_size);

	bool need_index_range = (buffer_view.target == fx::gltf::BufferView::TargetType::ElementArrayBuffer);
	if (need_index_range) {
		if (accessor.min.size() == 1 && accessor.max.size() == 1) {
			res.min_idx = accessor.min[0];
			res.max_idx = accessor.max[0];
		}
	}

	for(uint32_t i = 0; i < accessor.count; ++i) {
		for (uint32_t j = 0; j < dtype_size; ++j) {
			uint32_t offset = buffer_view.byteOffset + accessor.byteOffset + i * buffer_stride + j;
		        res.data.push_back(buffer.data[offset]);
		}
	}

	res.comp_type = (uint32_t)accessor.componentType;
	res.comp_count = calc_comp_count(accessor);
	res.elem_byte_size = dtype_size;
	res.elem_count = accessor.count;

	return res;
}


static rc::model::attr_description_t load_gltf_primitive_attr(const fx::gltf::Document& doc,
					  const fx::gltf::Primitive& prim,
					  std::string name)
{
	rc::model::attr_description_t res;
	auto attr_pos = prim.attributes.find(name);
	if (attr_pos != prim.attributes.end()) {
		auto idx = attr_pos->second;
		res = gltf_attr_data(idx, doc);
		res.name = name;
	}
	return res;
}


static rc::model::attr_description_t load_gltf_primitive_indices(
	const fx::gltf::Document& doc,
	const fx::gltf::Primitive& prim)
{
	rc::model::attr_description_t res;
	if (prim.indices >= 0) {
		res = gltf_attr_data(prim.indices, doc);
		res.name = "INDEX";
	}
	return res;
}


static std::vector<std::pair<model::Mesh, int>> load_gltf_mesh(const fx::gltf::Mesh& mesh, const fx::gltf::Document& doc)
{
	ZoneScoped;
	std::vector<std::pair<model::Mesh, int>> res;

	for (const auto& primitive : mesh.primitives) {

		std::vector<rc::model::attr_description_t> attrs;

		for (auto& attr : primitive.attributes) {
			attrs.push_back(load_gltf_primitive_attr(doc, primitive, attr.first));
		}

		auto m = model::Mesh(mesh.name);

		m.upload_data(load_gltf_primitive_indices(doc, primitive), std::move(attrs));
		m.draw_mode = static_cast<uint32_t>(primitive.mode);

		res.emplace_back(std::make_pair(std::move(m), primitive.material));
	}
	return res;
}

struct node_transform {

	zcm::vec3 translate;
	zcm::vec3 scale{1.0f};
	zcm::quat rotate;

	node_transform() = default;
	explicit node_transform(const fx::gltf::Node& node) {
		scale.x = node.scale[0];
		scale.y = node.scale[1];
		scale.z = node.scale[2];

		translate.x = node.translation[0];
		translate.y = node.translation[1];
		translate.z = node.translation[2];

		rotate.x = node.rotation[0];
		rotate.y = node.rotation[1];
		rotate.z = node.rotation[2];
		rotate.w = node.rotation[3];
	}

	node_transform operator*(const node_transform& other) const {
		node_transform res;
		res.translate = translate + other.translate;
		res.scale = scale * other.scale;
		res.rotate = rotate * other.rotate;
		return res;
	}

};


static void load_gltf_mesh_with_material(model::data& res,
                                         std::map<int, int>& materials_cache,
                                         const std::filesystem::path& material_path,
                                         const fx::gltf::Document& doc,
                                         const node_transform& transform,
                                         size_t mesh_id) {
	ZoneScoped;

	const auto& mesh = doc.meshes[mesh_id];
	auto meshes_materials = load_gltf_mesh(mesh, doc);

	for (auto&& mm : meshes_materials) {

		res.primitives.push_back(std::move(mm.first));
		res.scale.push_back(transform.scale);
		res.translate.push_back(transform.translate);
		res.rotation.push_back(transform.rotate);

		auto matcache_pos = materials_cache.find(mm.second);

		if (matcache_pos != materials_cache.end()) {
			res.primitive_material.push_back(matcache_pos->second);
		} else {
			res.primitive_material.push_back(res.materials.size());
			materials_cache.insert(std::make_pair(mm.second, (int)res.materials.size()));
			res.materials.push_back(load_gltf_material(doc, mm.second, material_path));
		}
	}
}



static void load_node_recursive(model::data& res,
                                const fx::gltf::Document& doc,
                                std::map<int, int>& materials_cache,
                                const std::filesystem::path& material_path,
                                const node_transform& parent_transform,
                                size_t node_idx)
{
	ZoneScoped;
	const auto& node = doc.nodes.at(node_idx);
	auto transform = parent_transform * node_transform{node};

	for (auto ch : node.children)
		load_node_recursive(res, doc, materials_cache, material_path, transform, ch);

	if (node.mesh >= 0) {

		zcm::vec3 node_scale{zcm::no_init_t{}}, node_translate{zcm::no_init_t{}};
		zcm::quat node_rotate{zcm::no_init_t{}};



		load_gltf_mesh_with_material(res,
		                             materials_cache,
		                             material_path,
		                             doc,
		                             transform,
		                             node.mesh);
	}
}


bool model::load_gltf_file(data& res, const std::filesystem::path& path)
{
	ZoneScoped;

	std::map<int, int> materials_cache;

	fx::gltf::Document doc;
	{
		ZoneScopedN("parse gltf file");
		doc = fx::gltf::LoadFromText(path, fx::gltf::ReadQuotas{32, 1024*1024*1024, 1024*1024*1024});
	}

	auto material_path = path.parent_path();

	if (doc.scenes.empty()) {

		for (size_t i = 0; i < doc.meshes.size(); ++i) {

			load_gltf_mesh_with_material(res,
			                             materials_cache,
			                             material_path,
			                             doc,
			                             node_transform{},
			                             i);

		}

	}


	for (const auto& scene : doc.scenes) {

		for (const auto& node_idx : scene.nodes) {
			// FixMe: rewrite non-recursively
			load_node_recursive(res, doc, materials_cache, material_path, node_transform{}, node_idx);
		}
	}
	return true;
}


enum class AttrIndex
{
	Invalid = -1,
	Position = 0,
	Normal = 1,
	Tangent = 2,
	TexCoord0 = 3,
	TexCoord1 = 4,
	Color0 = 5,
	Joints0 = 8,
	Weights0 = 10
};

static AttrIndex get_attr_index(const std::string_view name)
{
	if (name == "POSITION")
		return AttrIndex::Position;
	if (name == "NORMAL")
		return AttrIndex::Normal;
	if (name == "TANGENT")
		return AttrIndex::Tangent;
	if (name == "TEXCOORD_0")
		return AttrIndex::TexCoord0;
	if (name == "TEXCOORD_1")
		return AttrIndex::TexCoord1;
	if (name == "COLOR_0")
		return AttrIndex::Color0;
	if (name == "JOINTS_0")
		return AttrIndex::Joints0;
	if (name == "WEIGHTS_0")
		return AttrIndex::Weights0;
	return AttrIndex::Invalid;
}


bool model::Mesh::valid() const noexcept
{
	return numverts != 0 && vao;
}

model::Mesh::Mesh(std::string name_) : name(std::move(name_))
{

}


void model::Mesh::upload_data(attr_description_t index, std::vector<attr_description_t> attrs) {
	ZoneScoped;
	TracyGpuZone("mesh_upload_data");

	// set up GPU objects --------------------------------------------------
	glCreateVertexArrays(1, vao.get());
	rcObjectLabel(vao, fmt::format("mesh vao: {}", name));
	if (!index.data.empty()) {
		glCreateBuffers(1, ebo.get());
		rcObjectLabel(ebo, fmt::format("mesh ebo: {}", name));
		glNamedBufferStorage(*ebo, index.data.size(), index.data.data(), gl::GL_NONE_BIT);
		index_type = index.comp_type;
		numverts = index.elem_count;

		index_min = index.min_idx;
		index_max = index.max_idx;

		glVertexArrayElementBuffer(*vao, *ebo);
	}

	std::vector<uint8_t> storage;

	attrs.erase(std::remove_if(attrs.begin(), attrs.end(), [](const auto& attr)
	{
		return get_attr_index(attr.name) == AttrIndex::Invalid;
	}), attrs.end());

	std::sort(attrs.begin(), attrs.end(), [](const auto& aa, const auto& ab)
	{
		return get_attr_index(aa.name) < get_attr_index(ab.name);
	});


	for (auto& attr : attrs) {
		attr.offset = storage.size();
		storage.insert(storage.end(), attr.data.begin(), attr.data.end());
	}

	if (storage.size() == 0) {
		numverts = 0;
		fmt::print(stderr, "[mesh] No vertex data for mesh '{}'!\n", name);
		return;
	}

	glCreateBuffers(1, vbo.get());
	rcObjectLabel(vbo, fmt::format("mesh vbo: {}", name));
	glNamedBufferStorage(*vbo, storage.size(), storage.data(), gl::GL_NONE_BIT);

	int binding_index = 0;
	for (auto& attr : attrs) {

		auto attr_index = get_attr_index(attr.name);
		assert(attr_index != AttrIndex::Invalid);

		if (attr_index == AttrIndex::Position) {
			bbox = attr.bbox;
			numverts_unique = attr.elem_count;
			if (numverts == 0) {
				numverts = attr.elem_count;
			}
		}

		if (attr_index == AttrIndex::Tangent)
			has_tangents = true;

		auto attribute_index = static_cast<GLuint>(attr_index);

		// set up VBO: binding index, vbo, offset, stride
		glVertexArrayVertexBuffer(*vao, binding_index, *vbo, attr.offset, attr.elem_byte_size);
		// assign VBOs to attributes
		glVertexArrayAttribBinding(*vao, attribute_index, binding_index);
		// specify attrib format: attrib idx, element count, format, normalized, relative offset
		glVertexArrayAttribFormat(*vao, attribute_index, attr.comp_count, static_cast<GLenum>(attr.comp_type), GL_FALSE, 0);
		// enable attrib
		glEnableVertexArrayAttrib(*vao, attribute_index);

		++binding_index;
	}
}
