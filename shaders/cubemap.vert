#version 440 core

out vec3 FragPos;

layout(location=0) uniform mat4 view_projection;


// ref: Williamson, D. (2016) 'Buffer-free Generation of Triangle Strip Cube Vertices',
//      in Lengyel, E. (ed) Game Engine Gems 3. CRC Press, pp. 85-89.
vec3 TriangleStripCubePosition(int vertex_id)
{
	// adapted for right-handed coordinate system
	int x = (0x287a >> vertex_id) & 1;
	int y = (0x31e3 >> vertex_id) & 1;
	int z = (0x02af >> vertex_id) & 1;
	return (vec3(x, y, z) - 0.5) * 2;
}

void main()
{
	vec3 aPos = TriangleStripCubePosition(gl_VertexID);
	FragPos = aPos;
	vec4 pos = view_projection * vec4(aPos, 1.0);
	gl_Position =vec4(pos.xy, 0.0, pos.w); // NOTE: z-component should be 0.0 with reversed Z.
}
