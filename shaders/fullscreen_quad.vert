#version 450 core

layout(location=0) out vec2 TexCoords;

const vec2 data[4] = vec2[]
(
  vec2(-1.0,  1.0),
  vec2(-1.0, -1.0),
  vec2( 1.0,  1.0),
  vec2( 1.0, -1.0)
);

void main()
{
	vec2 FragPos = data[gl_VertexID];
	TexCoords = (FragPos + 1.0f) * 0.5;
	gl_Position = vec4(FragPos, 1.0, 1.0);
}
