#include <rendercat/cubemap.hpp>
#include <rendercat/uniform.hpp>
#include <stb_image.h>
#include <string>
#include <iostream>

#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>

using namespace gl45core;

Cubemap::Cubemap() noexcept
{
	static const float cubemap_vertices[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	GLuint vao, vbo;

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glNamedBufferStorage(vbo, sizeof(cubemap_vertices), &cubemap_vertices, GL_NONE_BIT);

	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3*sizeof(float));
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);
	m_vao = vao;
	m_vbo = vbo;
}

Cubemap::~Cubemap() noexcept
{
	glDeleteBuffers(1, &m_vbo);
	glDeleteVertexArrays(1, &m_vao);
	glDeleteTextures(1, &m_cubemap);
	glDeleteTextures(1, &m_envmap);
}

void Cubemap::load_textures(const std::initializer_list<std::string_view> & textures, std::string_view basedir)
{
	assert(textures.size() == 6);

	std::string path;
	path.reserve(basedir.size() + 20);

	GLuint tex;
	// BUG: AMD driver cannot upload faces of single cubemap, only cubemap array (size of 1 still works)
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &tex);
	glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	struct stb_guard
	{
		stb_guard()
		{
			stbi_set_flip_vertically_on_load(0);
		}
		~stb_guard()
		{
			stbi_set_flip_vertically_on_load(1);
		}
	};

	// because madness
	stb_guard guard;

	bool texture_storage_allocated{false};
	int face{0};
	int prev_face_width{0}, prev_face_height{0};
	for(const auto& texture : textures) {
		path.clear();
		path.append(basedir);
		path.append(texture);

		int face_width, face_height, chan;
		auto data = stbi_loadf(path.data(), &face_width, &face_height, &chan, 3);
		if(data && chan == 3) {
			if(!texture_storage_allocated) {
				glTextureStorage3D(tex, 1, GL_RGB16F, face_width, face_height, 6);
				texture_storage_allocated = true;
				prev_face_width = face_width;
				prev_face_height = face_height;
			} else {
				if(face_width != prev_face_width || face_height != prev_face_height) {
					std::cerr << "could not open cubemap face [" << texture << "] from \'" << basedir << "\'"
					          << ": face size does not match: current: " << face_width << 'x' << face_height
					          << "  previous: " << prev_face_width << 'x' << prev_face_height << std::endl;

					stbi_image_free(data);
					glDeleteTextures(1, &tex);
					return;
				}
			}

			glTextureSubImage3D(tex, 0, 0, 0, face, face_width, face_height, 1, GL_RGB, GL_FLOAT, data);
			++face;

			stbi_image_free(data);
		} else {
			std::cerr << "could not open cubemap face [" << texture << "] from \'" << basedir << "\'";
			if(chan != 3) {
				std::cerr << ": invalid channel count (should be 3): " << chan;
			}
			std::cerr << std::endl;
		}
	}

	m_cubemap = tex;
}

void Cubemap::draw(uint32_t shader, const glm::mat4 & view, const glm::mat4 & projection) noexcept
{
	glUseProgram(shader);

	unif::m4(shader, "view", glm::mat4(glm::mat3(view)));
	unif::m4(shader, "projection", projection);

	glBindVertexArray(m_vao);
	glBindTextureUnit(0, m_cubemap);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

