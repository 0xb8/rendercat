#pragma once
#include <debug_draw.hpp>

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

	zcm::mat4 mvpMatrix;
	zcm::vec2 window_size;

private:

	std::uint32_t linePointProgram;
	std::uint32_t linePointProgram_MvpMatrixLocation;

	std::uint32_t linePointVAO;
	std::uint32_t linePointVBO;

	static const char * linePointVertShaderSrc;
	static const char * linePointFragShaderSrc;

};

} // namespace rc
