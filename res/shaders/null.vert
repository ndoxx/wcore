#version 400 core
layout(location = 0) in vec3 in_position;

uniform mat4 m4_ModelViewProjection;

void main()
{
    gl_Position = m4_ModelViewProjection*vec4(in_position, 1.0);
}
