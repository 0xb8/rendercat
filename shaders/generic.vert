#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTexCoords;

out INTERFACE {
	vec3 FragPos;
	mat3 TBN;
	vec2 TexCoords;
} vs_out;

uniform mat4 model;
uniform mat4 proj_view;
uniform mat3 normal_matrix;

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
	gl_Position = proj_view * vec4(vs_out.FragPos, 1.0);
	vs_out.TexCoords = aTexCoords;

	vec3 N = normal_matrix * aNormal;
	vec3 T = normal_matrix * aTangent;
	vec3 B = cross(N,T);
	vs_out.TBN = mat3(T, B, N);
}
