#version 440 core

layout(location = 0) in  vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(binding=0) uniform sampler2DMS hdrBuffer;
layout(binding=1) uniform sampler2D bloomBuffer;

layout(location=0) uniform float exposure;
layout(location=1) uniform int   sample_count;
layout(location=2) uniform float bloom_strength;
layout(location=3) uniform ivec2  do_bloom_srgb;

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

vec3 toSrgb(vec3 color) {
    return max(1.055 * pow(color, vec3(0.416666667)) - 0.055, 0);
}

void main()
{
	vec3 W = Uncharted2Tonemap(vec3(11.2)); // white point
	vec3 res;

	vec3 bloom = vec3(0);
	if (bool(do_bloom_srgb[0])) {
		vec3 color = textureLod(bloomBuffer, TexCoords, 0).rgb;
		color *= bloom_strength;
		bloom = color;
	}

	for(int i = 0; i < sample_count; ++i) { // apply tonemapping per sample to avoid hard contrast aliasing
		vec3 color = textureMultisample(hdrBuffer, TexCoords, i).rgb;
		color += bloom;
		res += Uncharted2Tonemap(color * exposure * 2);
	}
	res /= sample_count * W;

    if (bool(do_bloom_srgb[1])) {
        res = toSrgb(res);
    }

	//vec3 tonemapped_color = Uncharted2Tonemap(color * exposure * 2.0);
	FragColor = vec4(res, 1.0);
}
