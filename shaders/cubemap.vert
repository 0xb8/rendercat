#version 440 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

layout(location=0) uniform mat4 view;
layout(location=1) uniform mat4 projection;

void main()
{
	TexCoords = aPos;
	vec4 pos = projection * view * vec4(aPos, 1.0);
	gl_Position =vec4(pos.xy, 0.0, pos.w); // NOTE: z-component should be 0.0 with reversed Z.
}
