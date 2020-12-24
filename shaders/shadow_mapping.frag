#version 450 core

#ifdef ALPHA_MASKED
const int MATERIAL_BASE_COLOR_MAP      = 1 << 1;
const int MATERIAL_ALPHA_MASK          = 1 << 15;

layout(binding=0) uniform sampler2D material_diffuse;

layout(location = 0) out	vec4 FragColor;

struct Material {
	vec4      base_color_factor;
	vec3      emission_factor;
	float     normal_scale;
	float     roughness;
	float     metallic;
	float     occlusion_strength;
	float     alpha_cutoff;
	int       type;
};

layout(std140, binding=0) uniform MaterialUBO {
	Material material;
};

layout(location = 0) in INTERFACE {
	vec2 TexCoords;
} fs_in;

vec4 getMaterialBaseColor()
{
	vec4 base_color = material.base_color_factor;
	if((material.type & MATERIAL_BASE_COLOR_MAP) != 0) {
		base_color *= texture(material_diffuse, fs_in.TexCoords);
		if(base_color.a < material.alpha_cutoff) {
			discard;
		}
	}
	return base_color;
}

void main() {
	FragColor = getMaterialBaseColor();
}

#else

void main()
{
    // gl_FragDepth = gl_FragCoord.z;
}

#endif
