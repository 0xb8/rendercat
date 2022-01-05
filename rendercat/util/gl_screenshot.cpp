#include <rendercat/util/gl_screenshot.hpp>
#include <glbinding/gl45core/functions.h>
#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/types.h>
#include <stb_image_write.h>
#include <cassert>

using namespace gl45core;


void rc::util::gl_screenshot(unsigned w, unsigned h, const std::filesystem::path& file)
{
	std::vector<uint8_t> data;
	data.resize(w * h * 3);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data.data());

	stbi_flip_vertically_on_write(true);
	stbi_write_png(file.u8string().data(), w, h, 3, data.data(), 0);
}

void rc::util::gl_save_hdr_texture(uint32_t tex, const std::filesystem::path& file)
{
	if(!glIsTexture(tex)) {
		fputs("gl_save_hdr_texture(): not a texture object", stderr);
		return;
	}

	GLint tex_format = 0;
	glGetTextureLevelParameteriv(tex, 0, GL_TEXTURE_INTERNAL_FORMAT, &tex_format);

	if(tex_format != GL_RGBA16F && tex_format != GL_RGBA32F) {
		fputs("gl_save_hdr_texture(): texture format not supported", stderr);
		return;
	}

	GLint tex_width = 0, tex_height = 0;
	glGetTextureLevelParameteriv(tex, 0, GL_TEXTURE_WIDTH, &tex_width);
	glGetTextureLevelParameteriv(tex, 0, GL_TEXTURE_HEIGHT, &tex_height);

	std::vector<float> buf;
	buf.resize(tex_width * tex_height * 4);

	auto bufsize = buf.size() * sizeof(float);
	glGetTextureImage(tex, 0, GL_RGBA, GL_FLOAT, static_cast<GLsizei>(bufsize), buf.data());

	stbi_flip_vertically_on_write(true);
	stbi_write_hdr(file.u8string().data(), tex_width, tex_height, 4, buf.data());
}
