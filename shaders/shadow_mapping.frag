#version 450 core
#include "constants.glsl"
#include "material.glsl"

layout(location = 0) uniform bool alpha_masked;

layout(location = 0) in INTERFACE {
	vec2 TexCoords;
} fs_in;


void main() {
	if(alpha_masked) {
		float alpha = texture(material_diffuse, fs_in.TexCoords).a;
		if (alpha < material.alpha_cutoff)
			discard;
	}
}
