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

const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const float W = 11.2;

vec3 Uncharted2Tonemap(vec3 x)
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	vec3 color = textureMultisample(hdrBuffer, ivec2(TexCoords * texture_size)).rgb;

	vec3 curr = Uncharted2Tonemap(exposure * 2.0 * color);
	// TODO: precumpute this value on CPU
	vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
	vec3 res = curr * whiteScale;


	//vec3 res = vec3(1.0) - exp(-color * exposure);
	FragColor = vec4(res, 1.0);
}
