#include <rendercat/mesh.hpp>
#include <fmt/core.h>

#include <zcm/vec3.hpp>
#include <zcm/vec4.hpp>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;
using namespace rc;


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


	// set up GPU objects --------------------------------------------------
	glCreateVertexArrays(1, vao.get());
	if (!index.data.empty()) {
		glCreateBuffers(1, ebo.get());
		glNamedBufferStorage(*ebo, index.data.size(), index.data.data(), GL_NONE_BIT);
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
	glNamedBufferStorage(*vbo, storage.size(), storage.data(), GL_NONE_BIT);

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
