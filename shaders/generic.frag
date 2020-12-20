#version 450 core

//layout(early_fragment_tests) in;
layout(binding=0) uniform sampler2D material_diffuse;
layout(binding=1) uniform sampler2D material_normal;
layout(binding=2) uniform sampler2D material_specular_map;
layout(binding=3) uniform sampler2D material_roughness_metallic;
layout(binding=4) uniform sampler2D material_occlusion;
layout(binding=5) uniform sampler2D material_emission;
layout(binding=32) uniform sampler2DShadow shadow_map;
layout(binding=33) uniform samplerCubeArray uReflection;
layout(binding=34) uniform samplerCubeArray uIrradiance;
layout(binding=35) uniform sampler2D uBRDFLut;


const int MATERIAL_BASE_COLOR_MAP      = 1 << 1;
const int MATERIAL_NORMAL_MAP          = 1 << 2;
const int MATERIAL_ROUGHNESS_METALLIC  = 1 << 3;
const int MATERIAL_OCCLUSION_MAP       = 1 << 4;
const int MATERIAL_EMISSION_MAP        = 1 << 5;
const int MATERIAL_SPECULAR_GLOSSINESS = 1 << 6;
const int MATERIAL_BLEND               = 1 << 14;
const int MATERIAL_ALPHA_MASK          = 1 << 15;

const int MAX_DYNAMIC_LIGHTS = 16;
const float PI = 3.14159265359;


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

struct DirectionalLight {
	vec4 color_intensity;
	vec3 direction;
};

struct ExponentialDirectionalFog {
	vec4 dir_inscattering_color;
	vec4 inscattering_color;
	vec4 direction; // .xyz - direction, .w - exponent
	float inscattering_density;
	float extinction_density;
	bool enabled;
};


struct PointLightData {
    vec4 position;  // .xyz - pos,   .w - radius
    vec4 color;     // .rgb - color, .a - luminous intensity (candela)
};

struct SpotLightData {
	vec4 position;  // .xyz - pos,   .w - radius
	vec4 color;     // .rgb - color, .a - luminous intensity (candela)
	vec4 direction; // .xyz - dir,   .w - angle scale
	vec4 angle_offset;
};

layout(std140, binding=1) uniform PerFrame_frag {
    mat4 proj_view;
    mat4 light_proj_view;
    vec3  camera_forward;
    float znear;
    vec3  viewPos;
    float _pading1;

    DirectionalLight directional_light;
    ExponentialDirectionalFog directional_fog;

    PointLightData point_light[MAX_DYNAMIC_LIGHTS];
    SpotLightData  spot_light[MAX_DYNAMIC_LIGHTS];

    int num_msaa_samples;
    bool shadows_enabled;
};


struct Light {
    vec4 colorIntensity;  // rgb, pre-exposed intensity
    vec3 l;
    float attenuation;
    float NoL;
};

struct PixelParams {
	vec3  n;
	vec3  v;
	vec3  diffuseColor;
	vec3  f0;
	vec3  energyCompensation;
	vec2  dfg;
	float NoV;
	float roughness;
	float perceptualRoughness;
	float ao;
};

layout(location = 0) in INTERFACE {
	vec4 FragPosLightSpace;
	vec3 FragPos;
	vec3 Normal;
	vec3 Tangent;
	vec2 TexCoords;
	flat float BitangentSign;
} fs_in;

layout(location = 0) out	vec4 FragColor;


layout(location = 7) uniform bool has_tangents;
layout(location = 8) uniform int num_point_lights;
layout(location = 9) uniform int num_spot_lights;
layout(location = 10) uniform int point_light_indices[MAX_DYNAMIC_LIGHTS];
layout(location = 10 + MAX_DYNAMIC_LIGHTS) uniform int spot_light_indices[MAX_DYNAMIC_LIGHTS];

// ------- PBR stuff -----------------------------------------------------------
// mostly from https://github.com/google/filament

float clampNoV(float NoV) {
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return max(NoV, 1e-4);
}

float D_GGX(float roughness, float NoH) {
	float oneMinusNoHSquared = 1.0 - NoH * NoH;
	float a = NoH * roughness;
	float k = roughness / (oneMinusNoHSquared + a * a);
	float d = k * k * (1.0 / PI);
	return d;
}

vec3 F_Schlick(vec3 f0, float VoH) {
	float f = pow(1.0 - VoH, 5.0);
	return f + f0 * (1.0 - f);
}

vec3 F_Schlick_IBL(vec3 F0, float cos_theta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

float V_SmithGGXCorrelated(float roughness, float NoV, float NoL) {
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
	float a2 = roughness * roughness;
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
	float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
	float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
	float v = 0.5 / max(lambdaV + lambdaL, 0.001);
	// a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
	// a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
	return v;
}

float Fd_Lambert() {
	return 1.0 / PI;
}

vec3 specularLobe(const PixelParams pixel, const Light light, const vec3 h,
                  float NoV, float NoL, float NoH, float LoH) {

	float D = D_GGX(pixel.roughness, NoH);
	float V = V_SmithGGXCorrelated(pixel.roughness, NoV, NoL);
	vec3  F = F_Schlick(pixel.f0, LoH);

	return (D * V) * F;
}

vec3 diffuseLobe(const PixelParams pixel) {
	return pixel.diffuseColor * Fd_Lambert();
}

vec3 surfaceShading(const PixelParams pixel, const Light light, float occlusion) {
	vec3 h = normalize(pixel.v + light.l);

	float NoV = pixel.NoV;
	float NoL = clamp(light.NoL, 0.0, 1.0);
	float NoH = clamp(dot(pixel.n, h), 0.0, 1.0);
	float LoH = clamp(dot(light.l, h), 0.0, 1.0);

	vec3 Fr = specularLobe(pixel, light, h, NoV, NoL, NoH, LoH);
	vec3 Fd = diffuseLobe(pixel);

	// The energy compensation term is used to counteract the darkening effect
	// at high roughness
	vec3 color = Fd + Fr * pixel.energyCompensation;

	return (color * light.colorIntensity.rgb) *
	       (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}

float calcDirectionalShadow(vec4 fragPosLightSpace, float NdotL)
{
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range
	vec2 shadowTexCoords = projCoords.xy * 0.5 + 0.5;
	// get depth of current fragment from light's perspective
	float currentDepth = (projCoords.z);
	// bias accounting the angle to surface
	float bias = max(0.005 * (1.0 - NdotL), 0.001);
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadow_map, 0);

	for(int x = -1; x <= 1; ++x)
	{
	    for(int y = -1; y <= 1; ++y)
	    {
	        shadow += texture(shadow_map, vec3(shadowTexCoords + vec2(x, y) * texelSize , currentDepth - bias)).r;
	    }
	}
	shadow /= 9.0;


	if(projCoords.z > 1.0)
		shadow = 0.0;

	return shadow;
}

vec4 getMaterialBaseColor()
{
	vec4 base_color = material.base_color_factor;
	if((material.type & MATERIAL_BASE_COLOR_MAP) != 0) {
		base_color *= texture(material_diffuse, fs_in.TexCoords);
		if((material.type & MATERIAL_ALPHA_MASK) != 0) {
			if(num_msaa_samples > 1) {
				base_color.a = (base_color.a - material.alpha_cutoff) / max(fwidth(base_color.a), 0.0001) + 0.5;

			} else if(base_color.a < material.alpha_cutoff) {
				discard;
			}
		}
	}
	return base_color;
}

void getMaterialRougnessMetallic(out float roughness, out float metallic)
{
	vec2 res = vec2(material.roughness, material.metallic);
	if ((material.type & MATERIAL_ROUGHNESS_METALLIC) != 0)
		// rougness and metallic is stored in RG channels
		res *= texture(material_roughness_metallic, fs_in.TexCoords).gb;
	roughness = res.x;
	metallic = res.y;
}

vec3 getMaterialEmission()
{
	vec3 res = material.emission_factor;
	if((material.type & MATERIAL_EMISSION_MAP) != 0) {
		res *= texture(material_emission, fs_in.TexCoords).rgb;
	}
	return res;
}

vec3 getNormal()
{
	if((material.type & MATERIAL_NORMAL_MAP) != 0) {
		vec3 sampled_normal = texture(material_normal, fs_in.TexCoords).rgb * 2.0 - 1.0;
		vec3 normal = normalize(sampled_normal * vec3(material.normal_scale, material.normal_scale, 1.0));

		mat3 TBN;
		if (has_tangents) {
			TBN = mat3(fs_in.Tangent,
				   cross(fs_in.Normal, fs_in.Tangent) * fs_in.BitangentSign,
				   fs_in.Normal);
		} else {
			vec3 pos_dx = dFdx(fs_in.FragPos);
			vec3 pos_dy = dFdy(fs_in.FragPos);
			vec2 tex_dx = dFdx(fs_in.TexCoords);
			vec2 tex_dy = dFdy(fs_in.TexCoords);
			vec3 t = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

			vec3 n = normalize(fs_in.Normal);
			t = normalize(t - n * dot(n, t));
			vec3 b = normalize(cross(n, t));

			TBN = mat3(t, b, n);
		}
		return normalize(TBN * normal);
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

float getMaterialAO()
{
	if ((material.type & MATERIAL_OCCLUSION_MAP) != 0) {
		return texture(material_occlusion, fs_in.TexCoords).r;
	}
	return 1.0;
}

vec3 applyOcclusion(const PixelParams pixel, vec3 color)
{
	if ((material.type & MATERIAL_OCCLUSION_MAP) != 0) {
		float occlusion = pixel.ao;
		color = mix(color, color * occlusion, material.occlusion_strength);
	}
	return color;
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

float linearizeDepth(in float depth)
{
	return znear / depth;
}

vec3 calcPBRDirect(const PixelParams pixel){

	// calc direct light
	Light direct_light;
	direct_light.colorIntensity = directional_light.color_intensity;
	direct_light.l = directional_light.direction;
	direct_light.NoL = clamp(dot(pixel.n, directional_light.direction), 0.0, 1.0);
	direct_light.attenuation = 1.0;

	float shadow;
	if (shadows_enabled)
		shadow = calcDirectionalShadow(fs_in.FragPosLightSpace, direct_light.NoL);
	else shadow = 1.0;

	return surfaceShading(pixel, direct_light, shadow);
}

vec3 calcPBRPoint(const PixelParams pixel) {

	vec3 color = vec3(0);
	for (int i = 0; i < num_point_lights; ++i) {

		PointLightData pl = point_light[point_light_indices[i]];

		Light point;
		point.colorIntensity = pl.color;

		vec3 lightv = pl.position.xyz - fs_in.FragPos;
		float lightDistance = length(lightv);
		point.l = lightv / lightDistance;

		point.NoL = dot(pixel.n, point.l);
		point.attenuation = distance_attenuation(lightDistance, pl.position.w);
		color += surfaceShading(pixel, point, 1.0);
	}
	return color;
}

vec3 calcPBRSpot(const PixelParams pixel) {

	vec3 color = vec3(0);
	for (int i = 0; i < num_spot_lights; ++i) {

		SpotLightData sl = spot_light[spot_light_indices[i]];

		Light spot;
		spot.colorIntensity = sl.color;

		vec3 lightv = sl.position.xyz - fs_in.FragPos;
		float lightDistance = length(lightv);
		spot.l = lightv / lightDistance;

		spot.NoL = dot(pixel.n, spot.l);
		spot.attenuation = distance_attenuation(lightDistance, sl.position.w);
		spot.attenuation *= direction_attenuation(spot.l, sl.direction.xyz, sl.direction.w, sl.angle_offset.x);

		color += surfaceShading(pixel, spot, 1.0);
	}
	return color;
}

float computeSpecularAO(float NoV, float visibility, float roughness) {
    return clamp(pow(NoV + visibility, exp2(-16.0 * roughness - 1.0)) - 1.0 + visibility, 0.0, 1.0);
}

vec3 calcIBL(PixelParams pixel)
{
	vec3 r = reflect(-pixel.v, pixel.n);
	float diffuse_AO = pixel.ao;
	float specular_AO = computeSpecularAO(pixel.NoV, diffuse_AO, pixel.roughness);

	vec3 diffuse_irradiance = texture(uIrradiance, vec4(pixel.n, 0)).rgb;
	vec3 Fd = pixel.diffuseColor * diffuse_irradiance * diffuse_AO;

	vec3 specular_radiance = textureLod(uReflection, vec4(r, 0),
	                                    pixel.perceptualRoughness * textureQueryLevels(uReflection)).rgb;
	vec3 specularColor = mix(pixel.dfg.xxx, pixel.dfg.yyy, pixel.f0);
	vec3 Fr = specularColor * specular_radiance;

	Fr *= specular_AO * pixel.energyCompensation;

	// horizon occlusion with falloff, should be computed for direct specular too
	float horizon = min(1.0 + dot(r, pixel.n), 1.0);
	Fr *= horizon * horizon;

	return applyOcclusion(pixel, Fr + Fd);
}

void main()
{
	vec3 viewRay = viewPos - fs_in.FragPos;
	float viewRayLength = length(viewRay);

	float material_roughness, material_metallic;
	getMaterialRougnessMetallic(material_roughness, material_metallic);

	vec4 material_base_color = getMaterialBaseColor();
	vec3 diffuse_color = (1.0 - material_metallic) * material_base_color.rgb;

	vec3 N = getNormal();
	vec3 V = viewRay / viewRayLength;

	PixelParams pixel;
	pixel.n = N;
	pixel.v = V;
	pixel.diffuseColor = diffuse_color;
	pixel.f0 = mix(vec3(0.04), material_base_color.rgb, material_metallic);
	pixel.NoV = abs(dot(N, V)) + 1e-5;
	pixel.roughness = material_roughness * material_roughness;
	pixel.perceptualRoughness = material_roughness;
	pixel.ao = getMaterialAO();
	pixel.dfg = textureLod(uBRDFLut, vec2(pixel.NoV, pixel.perceptualRoughness), 0.0).rg;
	pixel.energyCompensation = 1.0 + pixel.f0 * (1.0 / pixel.dfg.y - 1.0);

	vec3 lighting = calcIBL(pixel) + calcPBRDirect(pixel) + calcPBRPoint(pixel) + calcPBRSpot(pixel);
	lighting += getMaterialEmission();

	if(directional_fog.enabled) {
		lighting = calcFog(directional_fog, lighting, viewRayLength, V);
	}

	FragColor = vec4(lighting, material_base_color.a);
}
