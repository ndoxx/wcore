#version 400 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

out v_data
{
    vec3 Normal;
    vec2 texCoord;
    vec4 ViewSpace;
    vec4 FragPos;
}vertex;

struct Transform
{
    mat4 m4_View;
    mat4 m4_Model;
    mat4 m4_ModelView;
    mat4 m4_ModelViewProjection;
    mat3 m3_Normal;                // Normal matrix to transform normals
};

// UNIFORMS
uniform Transform tr;

void main()
{
    gl_Position = tr.m4_ModelViewProjection * vec4(in_position,1.0);

    vertex.ViewSpace = tr.m4_ModelView * vec4(in_position, 1.0);
    vertex.FragPos   = tr.m4_Model     * vec4(in_position, 1.0);
    vertex.Normal    = normalize(tr.m3_Normal * in_normal);
    vertex.texCoord  = in_texCoord;
}
