#include <rendercat/cubemap.hpp>
#include <rendercat/uniform.hpp>
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

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glNamedBufferStorage(vbo, sizeof(cubemap_vertices), &cubemap_vertices, 0);

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

	int face = 0;
	bool allocated = false;
	int prevwidth, prevheight;
	for(const auto& texture : textures) {
		path.clear();
		path.append(basedir);
		path.append(texture);

		int width, height, chan;
		auto data = stbi_loadf(path.data(), &width, &height, &chan, 3);
		if(data) {
			if(!allocated) {
				glTextureStorage3D(tex, 1, GL_RGB16F, width, height, 6);
				allocated = true;
				prevwidth = width;
				prevheight = height;
			} else {
				if(width != prevwidth || height != prevheight) {
					std::cerr << "cubemap face dimensions have to match!\n"
					          << "face [" << texture << "] " << width << 'x' << height << "\n"
					          << "but should be " << prevwidth << 'x' << prevheight << '\n';

					stbi_image_free(data);
					glDeleteTextures(1, &tex);
					return;
				}
			}

			glTextureSubImage3D(tex, 0, 0, 0, face, width, height, 1, GL_RGB, GL_FLOAT, data);
			++face;

			stbi_image_free(data);
		} else {
			std::cerr << "could not open cubemap face \'" << texture << "\' from \'" << basedir << "\'\n";
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

