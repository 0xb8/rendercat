#version 440 core
#pragma rc_external_defines

//layout(early_fragment_tests) in;

// cannot put samplers into material because we need explicit texture unit binding
layout(binding=0) uniform sampler2D material_diffuse;
layout(binding=1) uniform sampler2D material_normal;
layout(binding=2) uniform sampler2D material_specular_map;
layout(binding=3) uniform sampler2D material_roughness_metallic_occlusion;
layout(binding=4) uniform sampler2D material_emission;

const int MATERIAL_BLENDED          = 1 << 5;
const int MATERIAL_ALPHA_MASKED     = 1 << 6;
const int MATERIAL_DIFFUSE_MAPPED   = 1 << 7;
const int MATERIAL_NORMAL_MAPPED    = 1 << 8;
const int MATERIAL_SPECULAR_MAPPED  = 1 << 9;
const int MATERIAL_ROUGHNESS_MAPPED = 1 << 10;
const int MATERIAL_METALLIC_MAPPED  = 1 << 11;
const int MATERIAL_OCCLUSION_MAPPED = 1 << 12;
const int MATERIAL_EMISSION_MAPPED  = 1 << 13;

const int MAX_DYNAMIC_LIGHTS = 16;


struct Material {
	vec4      diffuse;
	vec4      specular; // .rgb - color, .a - shininess
	vec4      rougness_metallic_emission;
	float     alpha_cutoff;
	int       type;
};

struct DirectionalLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct ExponentialDirectionalFog {
	vec4 dir_inscattering_color;
	vec4 inscattering_color;
	vec4 direction; // .xyz - direction, .w - exponent
	float inscattering_density;
	float extinction_density;
	bool enabled;
};

struct PointLight {
	vec4 position; // .xyz - pos,   .w - radius
	vec4 color;    // .rgb - color, .a - luminous intensity (candela)
};

struct SpotLight {
	vec4 direction; // .xyz - dir,   .w - angle scale
	vec4 position;  // .xyz - pos,   .w - radius
	vec4 color;     // .rgb - color, .a - luminous intensity (candela)
	float angle_offset;
};

in INTERFACE {
	vec3 FragPos;
	vec3 Normal;
	vec3 Tangent;
	vec2 TexCoords;
	flat float BitangentSign;
} fs_in;

out	vec4 FragColor;

uniform vec3 viewPos;

uniform Material material;

uniform DirectionalLight directional_light;
uniform ExponentialDirectionalFog directional_fog;

uniform PointLight point_light[MAX_DYNAMIC_LIGHTS];
uniform SpotLight  spot_light[MAX_DYNAMIC_LIGHTS];

uniform int point_light_indices[MAX_DYNAMIC_LIGHTS];
uniform int spot_light_indices[MAX_DYNAMIC_LIGHTS];

uniform int num_point_lights;
uniform int num_spot_lights;
uniform int num_msaa_samples;

vec4 getMaterialDiffuse()
{
	if((material.type & MATERIAL_DIFFUSE_MAPPED) != 0) {
		vec4 diffuse = texture(material_diffuse, fs_in.TexCoords);
		if((material.type & MATERIAL_ALPHA_MASKED) != 0) {
			if(num_msaa_samples > 1) {
				diffuse.a = (diffuse.a - material.alpha_cutoff) / max(fwidth(diffuse.a), 0.0001) + 0.5;

			} else if(diffuse.a < material.alpha_cutoff) {
				discard;
			}
		} else {
			diffuse.a = 1.0;
		}
		return diffuse;
	} else {
		return material.diffuse;
	}
}

vec3 getMaterialSpecular()
{
	if((material.type & MATERIAL_SPECULAR_MAPPED) != 0) {
		return texture(material_specular_map, fs_in.TexCoords).rgb;
	} else {
		return material.specular.rgb;
	}
}

vec3 getNormal()
{
	if((material.type & MATERIAL_NORMAL_MAPPED) != 0) {
		// NOTE: texture conversion could be normalized, but doesn't affect the quality much
		vec3 normal = texture(material_normal, fs_in.TexCoords).rgb;
		mat3 TBN = mat3(fs_in.Tangent, fs_in.BitangentSign * cross(fs_in.Normal, fs_in.Tangent), fs_in.Normal);
		return normalize(TBN * (normal * 2.0 - 1.0));
	} else {
		// normalize interpolated normals to prevent issues with specular pixel flicker.
		return normalize(fs_in.Normal);
	}
}

vec3 calcFog(const in ExponentialDirectionalFog fog,
             const in vec3 fragColor,
             const in float fragDistance,
             const in vec3 viewDir)
{
	vec3 fogColor;
	if(fog.dir_inscattering_color.a > 0.0) {
		float directional_amount = max(dot(viewDir, fog.direction.xyz), 0.0);
		directional_amount *= fog.dir_inscattering_color.a;
		fogColor = mix(fog.inscattering_color.rgb, fog.dir_inscattering_color.rgb, pow(directional_amount, fog.direction.w));
	} else {
		fogColor = fog.inscattering_color.rgb;
	}
	float extinctionAmount   = 1.0 - exp2(-fragDistance * fog.extinction_density);
	float inscatteringAmount = 1.0 - exp2(-fragDistance * fog.inscattering_density);
	inscatteringAmount *= fog.inscattering_color.a;
	extinctionAmount *= fog.inscattering_color.a;
	return fragColor * (1.0 - extinctionAmount) + fogColor * inscatteringAmount;
}

// prevent light leaking from behind the face when normal mapped
float check_light_side(vec3 lightDir) {
	float x	= dot(fs_in.Normal, lightDir);
	const float y = 0.0;
	// ref: http://theorangeduck.com/page/avoiding-shader-conditionals
	return max(sign(x - y), 0.0);
}

float distance_attenuation(const in float dist, const in float radius)
{
	const float decay = 2.0f;

	// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
	// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
	return pow(clamp(1.0 - pow(dist/radius, 4.0), 0.0, 1.0), decay) / max(pow(dist, decay), 0.01);
}

float direction_attenuation(const in vec3 lightDir,
                            const in vec3 spotDir,
                            const in float angleScale,
                            const in float angleOffset)
{
	// ref: listing (4) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
	// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
	float att = clamp(dot(spotDir, lightDir) * angleScale + angleOffset, 0.0, 1.0);
	return att * att;
}

vec3 calcDirectionalLight(const in DirectionalLight light,
                          const in vec3 viewDir,
                          const in mat3 materialParams)
{
	vec3 lightDir = light.direction;
	vec3 color = materialParams[0];
	vec3 normal = materialParams[2];

	vec3 ambient = directional_light.ambient * color;
	vec3 diffuse = light.diffuse * color *  max(dot(normal, lightDir), 0.0);

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	// saturating dot product seems to greatly reduce specular pixel flicker
	// with MSAA, but does not solve it completely (due to other light sources' contribution).
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), material.specular.a);
	vec3 specular = spec * light.specular * materialParams[1];

	return ambient + (diffuse + specular)  * check_light_side(lightDir);
}

vec3 calcPointLight(const in PointLight light,
                    const in vec3 viewDir,
                    const in mat3 materialParams,
		    out vec3 lightDir)
{
	vec3 lightv = light.position.xyz - fs_in.FragPos;
	float lightDistance = length(lightv);
	lightDir = lightv / lightDistance;

	vec3 albedo = materialParams[0];
	vec3 mat_spec = materialParams[1];
	vec3 normal = materialParams[2];

	vec3 diffuse = albedo * max(dot(normal, lightDir), 0.0);

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), material.specular.a);
	vec3 specular = spec * mat_spec * check_light_side(lightDir);

	vec3 light_val = light.color.a * light.color.rgb;
	float att = distance_attenuation(lightDistance, light.position.w);
	return att * light_val * (diffuse + specular);
}

vec3 calcSpotLight(const in SpotLight light,
                   const in vec3 viewDir,
                   const in mat3 materialParams)
{
	PointLight pl;
	pl.position = light.position;
	pl.color = light.color;
	vec3 lightDir;
	vec3 pl_color = calcPointLight(pl, viewDir, materialParams, lightDir);
	float dir_att = direction_attenuation(lightDir, light.direction.xyz, light.direction.w, light.angle_offset);
	return pl_color * dir_att;
}

void main()
{
	vec3 viewRay = viewPos - fs_in.FragPos;
	float viewRayLength = length(viewRay);
	vec3 viewDir = viewRay / viewRayLength;

	vec4 diffuse = getMaterialDiffuse();
	mat3 materialParams = mat3(diffuse.rgb, getMaterialSpecular(), getNormal());

	vec3 directional = calcDirectionalLight(directional_light, viewDir, materialParams);
	vec3 point = vec3(0.0);
	vec3 spot = vec3(0.0);
	for(int i = 0; i < num_point_lights; ++i) {
		vec3 lightDir;
		point += calcPointLight(point_light[point_light_indices[i]], viewDir, materialParams, lightDir);
	}

	for(int i = 0; i < num_spot_lights; ++i)
		spot += calcSpotLight(spot_light[spot_light_indices[i]], viewDir, materialParams);

	vec3 orig_color = directional + point + spot;
	vec3 final_color;
	if(directional_fog.enabled) {
		final_color = calcFog(directional_fog, orig_color, viewRayLength, viewDir);
	} else {
		final_color = orig_color;
	}

	FragColor = vec4(final_color, diffuse.a);
}
