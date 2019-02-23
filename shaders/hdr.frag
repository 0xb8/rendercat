#version 440 core

in  vec2 TexCoords;
out vec4 FragColor;

layout(binding=0) uniform sampler2DMS hdrBuffer;

layout(location=0) uniform float exposure;
layout(location=1) uniform int   sample_count;

vec4 textureMultisample(sampler2DMS sampler, vec2 TexCoords, int i)
{
	ivec2 coord = ivec2(TexCoords * textureSize(hdrBuffer));
	return texelFetch(sampler, coord, i);
}

vec3 Uncharted2Tonemap(vec3 x)
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ACESFilm(vec3 x)
{
	const float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
	vec3 W = Uncharted2Tonemap(vec3(11.2)); // white point
	vec3 res;
	for(int i = 0; i < sample_count; ++i) { // apply tonemapping per sample to avoid hard contrast aliasing
		vec3 color = textureMultisample(hdrBuffer, TexCoords, i).rgb;
		res += Uncharted2Tonemap(color * exposure * 2);
	}
	res /= sample_count * W;

	//vec3 tonemapped_color = Uncharted2Tonemap(color * exposure * 2.0);

	FragColor = vec4(res, 1.0);
}
