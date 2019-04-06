#pragma once

#include <cstdint>
#include <zcm/vec2.hpp>
#include <zcm/vec3.hpp>
#include <zcm/mat4.hpp>

#define DEBUG_DRAW_VEC3_TYPE_DEFINED
using ddVec3    = zcm::vec3;
using ddVec3_In  = const ddVec3&;
using ddVec3_Out = ddVec3&;

#include "debug-draw/debug_draw.hpp"

namespace rc {

class DDRenderInterfaceCoreGL final : public dd::RenderInterface
{
public:

	void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled) override;
	void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled) override;
	void drawGlyphList(const dd::DrawVertex * glyphs, int count, dd::GlyphTextureHandle glyphTex) override;
	dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void * pixels) override;

	DDRenderInterfaceCoreGL();
	~DDRenderInterfaceCoreGL();

	void setupShaderPrograms();
	void setupVertexBuffers();

	zcm::mat4 mvpMatrix;
	zcm::vec2 window_size;

private:

	std::uint32_t linePointProgram = 0;
	std::int32_t linePointProgram_MvpMatrixLocation = -1;

	std::uint32_t textProgram = 0;
        std::int32_t textProgram_GlyphTextureLocation = -1;
	std::int32_t textProgram_ScreenDimensions = -1;

	std::uint32_t linePointVAO = 0;
	std::uint32_t linePointVBO = 0;

	std::uint32_t textVAO = 0;
	std::uint32_t textVBO = 0;

	static const char * linePointVertShaderSrc;
	static const char * linePointFragShaderSrc;
	static const char * textVertShaderSrc;
	static const char * textFragShaderSrc;

};

} // namespace rc
