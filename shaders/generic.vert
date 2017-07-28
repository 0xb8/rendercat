#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out	vec3 FragPos;
out	mat3 TBN;
out	vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normal_matrix;
uniform mat4 light_mvp;

void main()
{
	FragPos = vec3(model * vec4(aPos, 1.0));
	gl_Position = projection * view * vec4(FragPos, 1.0);
	TexCoords = aTexCoords;

	vec3 N = normalize(normal_matrix * aNormal);
	vec3 T = normalize(normal_matrix * aTangent);
	vec3 B = normalize(cross(N, T));

	TBN = mat3(T, B, N);
}
