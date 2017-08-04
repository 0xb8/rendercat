#version 330 core
//layout(early_fragment_tests) in;

struct Material {
	sampler2D diffuse;
	sampler2D normal;
	sampler2D specular_map;
	vec3      specular;
	float     shininess;
	int       type;

};

struct DirectionalLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float radius;
	float intensity; // luminous intensity (candela)
};

in INTERFACE {
	vec3 FragPos;
	mat3 TBN;
	vec2 TexCoords;
} fs_in;

out	vec4 FragColor;


uniform vec3 viewPos;
uniform Material material;
uniform DirectionalLight directional_light;

const   int MAX_LIGHTS = 16;
uniform int num_active_lights;
uniform PointLight point_light[MAX_LIGHTS];
uniform int active_light_indices[MAX_LIGHTS];

const int MATERIAL_NORMAL_MAPPED = 1 << 8;
const int MATERIAL_SPECULAR_MAPPED = 1 << 9;

vec3 getMaterialSpecular()
{
	if((material.type & MATERIAL_SPECULAR_MAPPED) != 0) {
		return texture(material.specular_map, fs_in.TexCoords).rgb;
	} else {
		return material.specular;
	}
}

vec3 getNormal()
{
	if((material.type & MATERIAL_NORMAL_MAPPED) != 0) {
		vec3 normal = texture(material.normal, fs_in.TexCoords).rgb;
		return normalize(fs_in.TBN * normalize(normal * 2.0 - 1.0));
	} else {
		// normalize interpolated normals to prevent issues with specular pixel flicker.
		return normalize(fs_in.TBN[2]);
	}
}


vec3 calcDirectionalLight(const in DirectionalLight light,  const in vec3 viewDir, const in mat3 materialParams)
{
	vec3 lightDir = normalize(light.direction);
	vec3 color = materialParams[0];
	vec3 normal = materialParams[2];

	vec3 ambient = directional_light.ambient * color;
	vec3 diffuse = light.diffuse * color *  max(dot(normal, lightDir), 0.0);

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	// saturating dot product seems to greatly reduce specular pixel flicker
	// with MSAA, but does not solve it completely (due to other light sources' contribution).
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), material.shininess);
	vec3 specular = spec * light.specular * materialParams[1];

	return ambient + diffuse + specular;
}

float point_light_falloff(const in float dist, const in float radius)
{
	const float decay = 2.0f;

	// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
	// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
	return pow(clamp(1.0 - pow(dist/radius, 4.0), 0.0, 1.0), decay) / max(pow(dist, decay), 0.01);
}

float point_light_bad_falloff(const in float dist, const in float rad)
{
	return 1.0 / (1.0 + 0.35 * dist + 0.44 * (dist * dist));
}

vec3 calcPointLight(const in PointLight light, const in vec3 viewDir, const in mat3 materialParams)
{
	vec3 lightv = light.position - fs_in.FragPos;
	vec3 lightDir = normalize(lightv);
	vec3 color = materialParams[0];
	vec3 normal = materialParams[2];
	vec3 diffuse = color * light.intensity * light.diffuse * max(dot(normal, lightDir), 0.0);
	vec3 ambient = color * light.intensity * light.ambient;

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(clamp(dot(normal, halfwayDir), 0.0, 1.0), material.shininess);
	vec3 specular = light.intensity * light.specular * spec * materialParams[1];

	// attenuation
	float dist = length(lightv);
	float attenuation = point_light_falloff(dist, light.radius);

	return attenuation * (ambient + diffuse + specular);
}

void main()
{
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 normal = getNormal();
	vec3 materialSpecular = getMaterialSpecular();
	vec3 materialDiffuse = texture(material.diffuse, fs_in.TexCoords).rgb;
	mat3 materialParams = mat3(materialDiffuse, materialSpecular, normal);

	vec3 directional = calcDirectionalLight(directional_light, viewDir, materialParams);

	vec3 point = vec3(0.0);
	for(int i = 0; i < num_active_lights; ++i)
		point += calcPointLight(point_light[active_light_indices[i]], viewDir, materialParams);

	FragColor = vec4(directional + point, 1.0);
}

