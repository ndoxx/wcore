#version 400 core
layout(location = 0) in vec3 in_position;

uniform mat4 m4_ModelViewProjection;

out vec4 vertex_pos;

void main()
{
    vertex_pos = m4_ModelViewProjection*vec4(in_position, 1.0);
    gl_Position = vertex_pos;
}
