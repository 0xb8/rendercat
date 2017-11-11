
// ================================================================================================
// -*- C++ -*-
// File:   sample_gl_core.cpp
// Author: Guilherme R. Lampert
// Brief:  Debug Draw usage sample with Core Profile OpenGL (version 3+)
//
// This software is in the public domain. Where that dedication is not recognized,
// you are granted a perpetual, irrevocable license to copy, distribute, and modify
// this file as you see fit.
// ================================================================================================


#include <debug_draw.hpp>
#include <cassert>

#include <glbinding/gl45core/enum.h>
#include <glbinding/gl45core/functions.h>
#include <glbinding/gl45core/bitfield.h>
#include <glbinding/gl45core/boolean.h>
#include <glbinding/gl45core/types.h>
#include <glm/gtc/type_ptr.hpp>

using namespace gl45core;

const char * DDRenderInterfaceCoreGL::linePointVertShaderSrc = "\n"
"#version 150\n"
"\n"
"in vec3 in_Position;\n"
"in vec4 in_ColorPointSize;\n"
"\n"
"out vec4 v_Color;\n"
"uniform mat4 u_MvpMatrix;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position  = u_MvpMatrix * vec4(in_Position, 1.0);\n"
"    gl_PointSize = in_ColorPointSize.w;\n"
"    v_Color      = vec4(in_ColorPointSize.xyz, 1.0);\n"
"}\n";

const char * DDRenderInterfaceCoreGL::linePointFragShaderSrc = "\n"
"#version 150\n"
"\n"
"in  vec4 v_Color;\n"
"out vec4 out_FragColor;\n"
"\n"
"void main()\n"
"{\n"
"    out_FragColor = v_Color;\n"
"}\n";


void DDRenderInterfaceCoreGL::drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled)
{
	assert(points != nullptr);
	assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

	glBindVertexArray(linePointVAO);
	glUseProgram(linePointProgram);

	glUniformMatrix4fv(linePointProgram_MvpMatrixLocation,
	                   1, GL_FALSE, glm::value_ptr(mvpMatrix));

	auto depthEnabledOld = glIsEnabled(GL_DEPTH_TEST);
	if (depthEnabled) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}

	// NOTE: Could also use glBufferData to take advantage of the buffer orphaning trick...
	glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(dd::DrawVertex), points);

	// Issue the draw call:
	glDrawArrays(GL_POINTS, 0, count);

	glUseProgram(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (depthEnabledOld) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}

}

void DDRenderInterfaceCoreGL::drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled)
{
	assert(lines != nullptr);
	assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

	glBindVertexArray(linePointVAO);
	glUseProgram(linePointProgram);

	glUniformMatrix4fv(linePointProgram_MvpMatrixLocation,
	                   1, GL_FALSE, glm::value_ptr(mvpMatrix));

	auto depthEnabledOld = glIsEnabled(GL_DEPTH_TEST);
	if (depthEnabled) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}

	// NOTE: Could also use glBufferData to take advantage of the buffer orphaning trick...
	glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(dd::DrawVertex), lines);

	// Issue the draw call:
	glDrawArrays(GL_LINES, 0, count);

	glUseProgram(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (depthEnabledOld) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
}


DDRenderInterfaceCoreGL::DDRenderInterfaceCoreGL()
        : mvpMatrix(glm::mat4())
        , linePointProgram(0)
        , linePointProgram_MvpMatrixLocation(-1)
        , linePointVAO(0)
        , linePointVBO(0)
{
	// Default OpenGL states:
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// This has to be enabled since the point drawing shader will use gl_PointSize.
	glEnable(GL_PROGRAM_POINT_SIZE);

	setupShaderPrograms();
	setupVertexBuffers();
}

DDRenderInterfaceCoreGL::~DDRenderInterfaceCoreGL()
{
	glDeleteProgram(linePointProgram);

	glDeleteVertexArrays(1, &linePointVAO);
	glDeleteBuffers(1, &linePointVBO);

}

void DDRenderInterfaceCoreGL::setupShaderPrograms()
{
	GLuint linePointVS = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(linePointVS, 1, &linePointVertShaderSrc, nullptr);
	glCompileShader(linePointVS);

	GLint linePointFS = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(linePointFS, 1, &linePointFragShaderSrc, nullptr);
	glCompileShader(linePointFS);

	linePointProgram = glCreateProgram();
	glAttachShader(linePointProgram, linePointVS);
	glAttachShader(linePointProgram, linePointFS);

	glBindAttribLocation(linePointProgram, 0, "in_Position");
	glBindAttribLocation(linePointProgram, 1, "in_ColorPointSize");
	glLinkProgram(linePointProgram);

	linePointProgram_MvpMatrixLocation = glGetUniformLocation(linePointProgram, "u_MvpMatrix");
	assert(linePointProgram_MvpMatrixLocation >= 0 && "Unable to get u_MvpMatrix uniform location!");
}

void DDRenderInterfaceCoreGL::setupVertexBuffers()
{
	glGenVertexArrays(1, &linePointVAO);
	glGenBuffers(1, &linePointVBO);

	glBindVertexArray(linePointVAO);
	glBindBuffer(GL_ARRAY_BUFFER, linePointVBO);

	// RenderInterface will never be called with a batch larger than
	// DEBUG_DRAW_VERTEX_BUFFER_SIZE vertexes, so we can allocate the same amount here.
	glBufferData(GL_ARRAY_BUFFER, DEBUG_DRAW_VERTEX_BUFFER_SIZE * sizeof(dd::DrawVertex), nullptr, GL_STREAM_DRAW);

	// Set the vertex format expected by 3D points and lines:
	std::size_t offset = 0;

	glEnableVertexAttribArray(0); // in_Position (vec3)
	glVertexAttribPointer(
				/* index     = */ 0,
				/* size      = */ 3,
				/* type      = */ GL_FLOAT,
				/* normalize = */ GL_FALSE,
				/* stride    = */ sizeof(dd::DrawVertex),
				/* offset    = */ reinterpret_cast<void *>(offset));
	offset += sizeof(float) * 3;

	glEnableVertexAttribArray(1); // in_ColorPointSize (vec4)
	glVertexAttribPointer(
				/* index     = */ 1,
				/* size      = */ 4,
				/* type      = */ GL_FLOAT,
				/* normalize = */ GL_FALSE,
				/* stride    = */ sizeof(dd::DrawVertex),
				/* offset    = */ reinterpret_cast<void *>(offset));

	// VAOs can be a pain in the neck if left enabled...
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
