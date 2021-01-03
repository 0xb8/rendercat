layout(binding=0) uniform sampler2D material_diffuse;
layout(binding=1) uniform sampler2D material_normal;
layout(binding=2) uniform sampler2D material_specular_map;
layout(binding=3) uniform sampler2D material_roughness_metallic;
layout(binding=4) uniform sampler2D material_occlusion;
layout(binding=5) uniform sampler2D material_emission;

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


