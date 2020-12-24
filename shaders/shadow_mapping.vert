#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoords;

layout(location = 0) uniform mat4 proj_view;
layout(location = 1) uniform mat4 model;

#ifdef ALPHA_MASKED
layout(location=0) out INTERFACE {
	vec2 TexCoords;
} vs_out;
#endif

void main()
{
	gl_Position = proj_view * model * vec4(aPos, 1.0);
	#ifdef ALPHA_MASKED
		vs_out.TexCoords = aTexCoords;
	#endif
}
