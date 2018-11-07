#version 400 core
layout(location = 0) in vec3 in_position;

struct Transform
{
    mat4 m4_ModelViewProjection;
};

uniform Transform tr;

void main()
{
    gl_Position = tr.m4_ModelViewProjection * vec4(in_position,1.0);
}
