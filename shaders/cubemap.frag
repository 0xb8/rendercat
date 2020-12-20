#version 440 core

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec3 FragPos;

layout(binding=0) uniform samplerCubeArray skybox;

layout(location=1) uniform int mip_level;

void main()
{
	// BUG: in AMD drivers. Workaround is to use cubemap array.
	FragColor = textureLod(skybox, vec4(FragPos, 0), mip_level);
}
