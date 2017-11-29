#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aTexCoords; // xy - UVs, z - bitangent sign

out INTERFACE {
	vec3 FragPos;
	vec3 Normal;
	vec3 Tangent;
	vec2 TexCoords;
	flat float BitangentSign;
} vs_out;

uniform mat4 model;
uniform mat4 proj_view;
uniform mat3 normal_matrix;

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
	gl_Position = proj_view * vec4(vs_out.FragPos, 1.0);
	vs_out.TexCoords = aTexCoords.xy;

	vs_out.Normal = normal_matrix * aNormal;
	vs_out.Tangent = normal_matrix * aTangent;
	vs_out.BitangentSign = aTexCoords.z;
}
