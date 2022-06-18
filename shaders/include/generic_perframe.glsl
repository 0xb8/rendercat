struct DirectionalLight {
	vec4 color_intensity;
	vec3 direction;
	float ambient_intensity;
};

struct ExponentialDirectionalFog {
	vec4 dir_inscattering_color;
	vec4 direction; // .xyz - direction, .w - exponent
	float inscattering_opacity;
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

layout(std140, binding=1) uniform PerFrame {
	mat4  proj_view;
	mat4  light_proj_view;
	vec3  camera_forward;
	float znear;
	vec3  viewPos;
	uint  per_frame_flags;

	DirectionalLight directional_light;
	ExponentialDirectionalFog directional_fog;

	PointLightData point_light[MAX_DYNAMIC_LIGHTS];
	SpotLightData  spot_light[MAX_DYNAMIC_LIGHTS];

	int num_msaa_samples;
};
