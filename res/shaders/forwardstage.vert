#version 400 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

struct Transform
{
    mat4 m4_Model;
    mat4 m4_ModelViewProjection;
    mat3 m3_Normal; // Normal matrix to transform normals
};

out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_texCoord;

// UNIFORMS
uniform Transform tr;

void main()
{
    vec4 worldPos   = tr.m4_Model * vec4(in_position, 1.0);
    vertex_pos      = worldPos.xyz/worldPos.w;
    vertex_normal   = normalize(tr.m3_Normal * in_normal);
    vertex_texCoord = in_texCoord;

    gl_Position = tr.m4_ModelViewProjection * vec4(in_position,1.0);
}
