#version 440 core
out vec4 FragColor;

in vec3 TexCoords;

layout(binding=0) uniform samplerCubeArray skybox;

void main()
{
	// BUG: in AMD drivers. Workaround is to use cubemap array.
	FragColor = texture(skybox, vec4(TexCoords,0));
}
