#include <cubemap.hpp>
#include <GL/glew.h>
#include <uniform.hpp>
#include <stb_image.h>
#include <string>
#include <iostream>

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

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glNamedBufferStorageEXT(vbo, sizeof(cubemap_vertices), &cubemap_vertices, 0);
	glEnableVertexArrayAttribEXT(vao, 0);
	glVertexArrayVertexAttribOffsetEXT(vao, vbo, 0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
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
	std::string path;
	path.reserve(basedir.size() + 20);

	GLuint tex;
	glGenTextures(1, &tex);
	glTextureParameteriEXT(tex, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteriEXT(tex, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteriEXT(tex, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteriEXT(tex, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteriEXT(tex, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// because madness
	stbi_set_flip_vertically_on_load(0);
	int face = 0;
	for(const auto& texture : textures) {

		path.clear();
		path.append(basedir);
		path.append(texture);

		int width, height, chan;
		auto data = stbi_loadf(path.data(), &width, &height, &chan, 3);
		if(data) {
			const auto format = GL_RGBA16F; //  GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;// - seems to reduce memory usage by ~50 MiB but has artifacts

			glTextureImage2DEXT(tex, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face++, 0, format, width, height, 0, GL_RGB, GL_FLOAT, data);
			stbi_image_free(data);
		} else {
			std::cerr << "could not open cubemap face \'" << texture << "\' from \'" << basedir << "\'\n";
		}
	}

	stbi_set_flip_vertically_on_load(1);

	m_cubemap = tex;
}

void Cubemap::draw(uint32_t shader, const glm::mat4 & view, const glm::mat4 & projection) noexcept
{
	glUseProgram(shader);

	unif::m4(shader, "view", glm::mat4(glm::mat3(view)));
	unif::m4(shader, "projection", projection);

	glBindVertexArray(m_vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

