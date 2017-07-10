#version 330 core

struct Material {
	sampler2D diffuse;
	sampler2D normal;
	vec3      specular;
	float     shininess;
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

	float constant;
	float linear;
	float quadratic;
};

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform DirectionalLight directional_light;
uniform PointLight point_light;

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{

	vec3 lightDir = normalize(-light.direction);
	float diff = max(dot(normal, lightDir), 0.0);

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

	// specular (phong)
	// vec3 reflectDir = reflect(-lightDir, normal);
	// float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

	vec3 ambient = directional_light.ambient * texture(material.diffuse, TexCoords).rgb;
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;
	vec3 specular = light.specular * spec * material.specular;
	return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - FragPos);
	float diff = max(dot(normal, lightDir), 0.0);

	// specular (blinn-phong)
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

	// specular (phong)
	// vec3 reflectDir = reflect(-lightDir, normal);
	// float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

	// attenuation
	float dist    = length(light.position - FragPos);
	float attenuation = 1.0 / (light.constant + light.linear * dist+ light.quadratic * (dist * dist));

	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;
	vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
	vec3 specular = light.specular * spec * material.specular;

	return attenuation * (ambient + diffuse + specular);
}

void main()
{

	vec3 normal = normalize(Normal);// texture(material.normal, TexCoords).rgb;
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 directional = calcDirectionalLight(directional_light, normal, viewDir);
	vec3 point       = calcPointLight(point_light, normal, viewDir);

	FragColor = vec4(directional + point,1.0);
}

