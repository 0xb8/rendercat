#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#define DEBUG_DRAW_VEC3_TYPE_DEFINED
using ddVec3    = glm::vec3;
using ddVec3_In  = const ddVec3&;
using ddVec3_Out = ddVec3&;

#include "debug-draw/debug_draw.hpp"

namespace rc {

class DDRenderInterfaceCoreGL final : public dd::RenderInterface
{
public:

	void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled) override;
	void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled) override;

	DDRenderInterfaceCoreGL();
	~DDRenderInterfaceCoreGL();

	void setupShaderPrograms();
	void setupVertexBuffers();

	glm::mat4 mvpMatrix;
	glm::vec2 window_size;

private:

	std::uint32_t linePointProgram;
	std::uint32_t linePointProgram_MvpMatrixLocation;

	std::uint32_t linePointVAO;
	std::uint32_t linePointVBO;

	static const char * linePointVertShaderSrc;
	static const char * linePointFragShaderSrc;

};

} // namespace rc
