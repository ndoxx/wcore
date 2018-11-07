#version 400 core

struct material
{
    float f_roughness;
    vec4 v4_tint;
};

in vec3 vertex_pos;
in vec3 vertex_normal;
in vec2 vertex_texCoord;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec3 out_bright_color;

// UNIFORMS
uniform material mt;

void main()
{
    out_color = mt.v4_tint;
    out_bright_color = vec3(0.0,0.0,0.0);
}
