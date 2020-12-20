#version 450 core
layout (location = 0) in vec3 aPos;

layout(location = 0) uniform mat4 proj_view;
layout(location = 1) uniform mat4 model;

void main()
{
    gl_Position = proj_view * model * vec4(aPos, 1.0);
}
