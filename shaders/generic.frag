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

in	vec3 FragPos;
in	mat3 TBN;
in	vec2 TexCoords;

out	vec4 FragColor;


uniform vec3 viewPos;
uniform Material material;
uniform DirectionalLight directional_light;



const   int MAX_LIGHTS = 8;
uniform int num_active_lights;
uniform PointLight point_light[MAX_LIGHTS];


vec3 getMaterialSpecular()
{
	if(material.has_specular_map != 0) {
		return texture(material.specular_map, TexCoords).rgb;
	} else {
		return material.specular;
	}
}

vec3 getNormal()
{
	if(material.has_normal_map != 0) {
		vec3 normal = texture(material.normal, TexCoords).rgb;
		return normalize(TBN * normalize(normal * 2.0 - 1.0));
	} else {
		return TBN[2];
	}
}


vec3 calcDirectionalLight(const in DirectionalLight light, const in vec3 normal, const in vec3 viewDir, const in vec3 materialSpecular)
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
	vec3 specular = light.specular * spec * materialSpecular;

	return ambient + (diffuse + specular);
}

float point_light_falloff(const in float dist, const in float radius)
{
	const float decay = 2.0f;

	// ref: equation (26) from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
	// @ https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/
	return pow(clamp(1.0 - pow(dist/radius, 4.0), 0.0, 1.0), decay) / max(pow(dist, decay), 0.01);
}

vec3 calcPointLight(const in PointLight light, const in vec3 normal, const in vec3 viewDir, const in vec3 materialSpecular)
{
	// attenuation
	float dist = length(light.position - FragPos);
	float attenuation = point_light_falloff(dist, light.radius);

	if(attenuation < 0.00001) // NOTE: does not actually speeds up, need to do this on CPU
		return vec3(0.0);

	vec3 lightDir = normalize(light.position - FragPos);

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.intensity * light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;
	vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
	vec3 specular = light.intensity * light.specular * spec * materialSpecular;

	return attenuation * (ambient + diffuse + specular);
}

void main()
{
	vec3 normal = getNormal();
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 materialSpecular = getMaterialSpecular();

	vec3 directional = calcDirectionalLight(directional_light, normal, viewDir, materialSpecular);
	vec3 point = vec3(0.0);
	for(int i = 0; i < num_active_lights; ++i)
		point += calcPointLight(point_light[i], normal, viewDir, materialSpecular);

	FragColor = vec4(directional + point, 1.0);
}

