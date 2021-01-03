#version 450 core
#extension GL_ARB_shader_viewport_layer_array: require

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoords;

layout(location = 0) uniform bool alpha_masked;
layout(location = 1) uniform mat4 model;
layout(location = 2) uniform int shadow_index;
layout(location = 4) uniform mat4 proj_view[6];

layout(location=0) out INTERFACE {
	vec2 TexCoords;
} vs_out;


void main()
{
#ifdef POINT_LIGHT
	gl_Position = proj_view[gl_InstanceID] * model * vec4(aPos, 1.0);
	gl_Layer = shadow_index * 6 + gl_InstanceID;
#else
	gl_Position = proj_view[0] * model * vec4(aPos, 1.0);
	gl_Layer = shadow_index;
#endif

	if (alpha_masked) {
		vs_out.TexCoords = aTexCoords;
	}
}
