#version 330 core

struct Material {
	sampler2D diffuse;
	sampler2D normal;
	sampler2D specular_map;
	vec3      specular;
	float     shininess;
	int       has_specular_map;
	int       has_normal_map;
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

out vec4 FragColor;


in	vec3 FragPos;
in	vec3 FragPosLightSpace;
in	vec3 Normal;
in	mat3 TBN;
in	vec2 TexCoords;


uniform vec3 viewPos;
uniform Material material;
uniform DirectionalLight directional_light;



const   int MAX_LIGHTS = 8;
uniform int num_active_lights;
uniform PointLight point_light[MAX_LIGHTS];


vec3 calcDirectionalLight(const in DirectionalLight light, const in vec3 normal, const vec3 viewDir)
{
	vec3 lightDir = normalize(light.direction);
	float diff = max(dot(normal, lightDir), 0.0);

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

	vec3 color = texture(material.diffuse, TexCoords).rgb;
	//float shadow = 1.0;//calcShadowMap();

	vec3 ambient = directional_light.ambient * color;
	vec3 diffuse = light.diffuse * diff * color;
	vec3 specular;

	if(material.has_specular_map) {
		specular = light.specular * spec * texture(material.specular_map, TexCoords).rgb;
	} else {
		specular = light.specular * spec * material.specular;
	}
	return ambient + (diffuse + specular);
}

float point_light_falloff(const in float dist, const in float radius)
{
	const float decay = 2.0f;


	// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
	// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
	return pow(clamp(1.0 - pow(dist/radius, 4.0), 0.0, 1.0), decay) / max(pow(dist, decay), 0.01);
}

vec3 calcPointLight(const in PointLight light, const in vec3 normal, const in vec3 viewDir)
{
	// attenuation
	float dist = length(light.position - FragPos);
	float attenuation = point_light_falloff(dist, light.radius);

	if(attenuation < 0.00001)
		return vec3(0.0);

	vec3 lightDir = normalize(light.position - FragPos);

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.intensity * light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;
	vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
	vec3 specular = light.intensity * light.specular * spec;

	if(material.has_specular_map) {
		specular *= texture(material.specular_map, TexCoords).rgb;
	} else {
		specular *= material.specular;
	}

	return attenuation * (ambient + diffuse + specular);
}

void main()
{
	vec3 normal;
	if(material.has_normal_map) {
		normal = texture(material.normal, TexCoords).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		normal = normalize(TBN * normal);
	} else {
		normal = TBN[2];
	}

	vec3 viewDir = normalize(viewPos - FragPos);

	vec3 directional = calcDirectionalLight(directional_light, normal, viewDir);
	vec3 point = vec3(0.0);
	for(int i = 0; i < num_active_lights; ++i)
		point += calcPointLight(point_light[i], normal, viewDir);

	FragColor = vec4(directional + point, 1.0);
}

