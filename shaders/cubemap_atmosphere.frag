#version 440 core
#include "constants.glsl"
#define ATMOSPHERE_SAMPLE_COUNT 16
#include "minimal_atmosphere.glsl"
#include "generic_perframe.glsl"

layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec3 FragPos;

layout(location=1) uniform bool u_DrawPlanet;


void main()
{
    vec3 rayStart  = viewPos;
    vec3 rayDir    = normalize(FragPos);
    float  rayLength = INFINITY;

    if (u_DrawPlanet) {
        vec2 planetIntersection = PlanetIntersection(rayStart, rayDir);
        if (planetIntersection.x > 0)
            rayLength = min(rayLength, planetIntersection.x);
    }

    vec3 lightDir   = directional_light.direction;
    vec3 lightColor = directional_light.color_intensity.rgb * abs(directional_light.color_intensity.w);

    vec3 transmittance;
    vec3 color = IntegrateScattering(rayStart, rayDir, rayLength, lightDir, lightColor, transmittance);


    FragColor =  vec4(color, 1);
}
