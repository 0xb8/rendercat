#version 330 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2DMS hdrBuffer;
uniform float exposure;
uniform ivec2 texture_size;
uniform int   sample_count;

vec4 textureMultisample(sampler2DMS sampler, ivec2 coord)
{
	vec4 color = vec4(0.0);
	if(sample_count != 0) {
		for (int i = 0; i < sample_count; ++i)
			color += texelFetch(sampler, coord, i);

		color /= float(sample_count);
	} else {
		color = texelFetch(sampler, coord, 0);
	}
	return color;
}

void main()
{
	vec3 color = textureMultisample(hdrBuffer, ivec2(TexCoords * texture_size)).rgb;
	vec3 res = vec3(1.0) - exp(-color * exposure);
	FragColor = vec4(res, 1.0);
}
