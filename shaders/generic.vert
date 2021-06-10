#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aTangent; // xyz - tangent, w - bitangent sign
layout (location = 3) in vec2 aTexCoords;

layout(location=0) out INTERFACE {
	vec4 FragPosLightSpace;
	vec3 FragPos;
	vec3 Normal;
	vec3 Tangent;
	vec2 TexCoords;
	flat float BitangentSign;
} vs_out;


layout(std140, binding=1) uniform PerFrame_vert {
    mat4 proj_view;
    mat4 light_proj_view;
};


layout(location=5) uniform mat4 model;
layout(location=6) uniform mat3 normal_matrix;
layout(location=7) uniform bool has_tangents;

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
	vs_out.FragPosLightSpace = light_proj_view * vec4(vs_out.FragPos, 1.0);
	gl_Position = proj_view * vec4(vs_out.FragPos, 1.0);
	vs_out.TexCoords = aTexCoords;

	vs_out.Normal = normalize(normal_matrix * aNormal);
	if (has_tangents) {
		vs_out.Tangent = normalize(normal_matrix * aTangent.xyz);
		vs_out.BitangentSign = aTangent.w;
	}
}
