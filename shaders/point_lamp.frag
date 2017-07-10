#version 330 core

out vec4 FragColor;

struct PointLight {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};


uniform PointLight point_light;

void main()
{

	FragColor = vec4(point_light.diffuse, 1.0);
}

