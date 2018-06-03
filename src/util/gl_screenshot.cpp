#include <rendercat/util/gl_screenshot.hpp>
#include <glbinding/gl45core/functions.h>
#include <glbinding/gl45core/enum.h>
#include <stb_image_write.h>
#include <cstring>

using namespace gl45core;

void rc::util::gl_screenshot(unsigned w, unsigned h, const char * filename)
{
	struct pixel
	{
		uint8_t r, g, b;
	};
	static_assert(sizeof(pixel) == 3, "bad alignment");

	std::vector<pixel> data;
	data.resize(w * h);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data.data());

	auto swap_pix = [](pixel& a, pixel& b)
	{
		pixel tmp;
		std::memcpy(&tmp, &a,   sizeof(pixel));
		std::memcpy(&a,   &b,   sizeof(pixel));
		std::memcpy(&b,   &tmp, sizeof(pixel));
	};

	for(size_t i = 0; i < h / 2; ++i) {
		size_t rowa = i*w, rowb = (h-i-1)*w;
		for(size_t j = 0; j < w; ++j) {
			swap_pix(data[rowa + j], data[rowb + j]);
		}
	}

	stbi_write_png(filename, w, h, 3, data.data(), 0);
}
