#version 400 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

out v_data
{
    vec3 Normal;
    //vec2 TexCoord;
    vec4 Color;
    vec4 ViewSpace;
    vec4 FragPos;
    //vec4 ShadowMapCoord0;
}vertex;

struct Transform
{
    mat4 m4_Model;
    mat4 m4_View;
    mat4 m4_ModelView;
    mat4 m4_ModelViewProjection;
    mat3 m4_Normal; // Normal matrix to transform normals
};

uniform Transform tr;

void main()
{
    gl_Position      = tr.m4_ModelViewProjection * vec4(in_position, 1.0);
    vertex.ViewSpace = tr.m4_ModelView           * vec4(in_position, 1.0);
    vertex.FragPos   = tr.m4_Model               * vec4(in_position, 1.0);

    //vertex.ShadowMapCoord0 = tr.lightMatTransformed * vec4(vertexPosition, 1.0);

    vertex.Normal          = normalize(tr.Normal * in_normal);
    //vertex.TexCoord        = vertexTexCoord;
    //vertex.Color           = vertexColor;
    vertex.Color = vec4(0.5,0.5,0.5,1.0);
}
