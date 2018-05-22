#include <rendercat/cubemap.hpp>
#include <rendercat/uniform.hpp>
#include <stb_image.h>
#include <string>
#include <utility>
#include <fmt/core.h>

#include <glbinding/gl/extension.h>
#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/types.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding-aux/Meta.h>
#include <rendercat/util/gl_meta.hpp>

using namespace gl45core;
using namespace rc;

static const char* const face_names_hdr[6] = {
	"right.hdr",
	"left.hdr",
	"top.hdr",
	"bottom.hdr",
	"front.hdr",
	"back.hdr"
};

static GLint max_texture_size;                       // spec requires 16384+, but some cards can only 8192
static GLboolean has_seamless_filtering;             // GL_ARB_seamless_cube_map is widely supported
static GLboolean has_seamless_filtering_per_texture; // requires GL_ARB_seamless_cubemap_per_texture, checked below

Cubemap::Cubemap()
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

	if(!max_texture_size) {
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_texture_size);
		has_seamless_filtering = glIsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		has_seamless_filtering_per_texture = rc::glmeta::extension_supported(gl::GLextension::GL_ARB_seamless_cubemap_per_texture);
		fmt::print("[cubemap] limits:\n"
		           "    Texture size:        {}\n"
		           "    Seamless filtering:\n"
		           "        Global:          {}\n"
		           "        Per-texture:     {}\n",
		           max_texture_size,

		           glbinding::aux::Meta::getString(has_seamless_filtering),
		           glbinding::aux::Meta::getString(has_seamless_filtering_per_texture));
	}

	if(!m_vao && !m_vbo) {
		GLuint vao, vbo;

		glCreateVertexArrays(1, &vao);
		glCreateBuffers(1, &vbo);
		glNamedBufferStorage(vbo, sizeof(cubemap_vertices), &cubemap_vertices, GL_NONE_BIT);

		glEnableVertexArrayAttrib(vao, 0);
		glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3*sizeof(float));
		glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(vao, 0, 0);
		m_vao.reset(vao);
		m_vbo.reset(vbo);
	}
}

void Cubemap::load_textures(std::string_view basedir)
{
	assert(basedir.length() > 0);
	std::string path;
	path.reserve(basedir.size() + 20);
	path.append(basedir);
	std::replace(path.begin(), path.end(),'\\', '/');
	if(path.back() != '/') {
		path.push_back('/');
	}
	size_t dirlen = path.length();

	rc::texture_handle tex;
	// BUG: AMD driver cannot upload faces of single cubemap, only cubemap array (size of 1 still works)
	glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, tex.get());
	glTextureParameteri(*tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(*tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(*tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	if(has_seamless_filtering_per_texture) {
		glTextureParameteri(*tex, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
	}

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
	for(int i = 0; i < 6; ++i) {
		path.resize(dirlen);
		path.append(face_names_hdr[i]);

		int face_width, face_height, chan;
		auto data = stbi_loadf(path.data(), &face_width, &face_height, &chan, 3);
		if(data && chan == 3) {
			if(!texture_storage_allocated) {
				glTextureStorage3D(*tex, 1, GL_RGB16F, face_width, face_height, 6);
				texture_storage_allocated = true;
				prev_face_width = face_width;
				prev_face_height = face_height;
			} else {
				if(face_width != prev_face_width || face_height != prev_face_height) {
					fmt::print(stderr, "[cubemap] bad face '{}': "
					           "face size does not match (current {}x{}, previous {}x{})\n",
					           path,
					           face_width,
					           face_height,
					           prev_face_width,
					           prev_face_height);

					stbi_image_free(data);
					return;
				}
			}

			glTextureSubImage3D(*tex, 0, 0, 0, face, face_width, face_height, 1, GL_RGB, GL_FLOAT, data);
			++face;

			stbi_image_free(data);
		} else {
			fmt::print(stderr, "[cubemap] could not open cubemap face '{}'\n", path);
			if(chan != 3) {
				fmt::print(stderr, "[cubemap] invalid channel count: {}\n", chan);
			}
			return;
		}
	}
	m_cubemap = std::move(tex);
}

void Cubemap::draw(uint32_t shader, const glm::mat4 & view, const glm::mat4 & projection) noexcept
{
	unif::m4(shader, 0, glm::mat4(glm::mat3(view)));
	unif::m4(shader, 1, projection);

	glUseProgram(shader);
	glBindVertexArray(*m_vao);
	glBindTextureUnit(0, *m_cubemap);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}
