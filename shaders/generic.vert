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
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normal_matrix;
uniform mat4 light_mvp;

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
	gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
	vs_out.TexCoords = aTexCoords;

	vec3 N = normalize(normal_matrix * aNormal);
	vec3 T = normalize(normal_matrix * aTangent);
	vec3 B = normalize(cross(N, T));

	vs_out.TBN = mat3(T, B, N);
}
