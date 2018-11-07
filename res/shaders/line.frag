#version 400 core

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec3 out_bright_color;

uniform vec4 v4_line_color;

void main()
{
    out_color = v4_line_color;
    out_bright_color = vec3(0.0,0.0,0.0);
}
