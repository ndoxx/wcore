#version 400 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

out v_data
{
    vec4 Color;
    vec2 texCoord;
}vertex;

struct Transform
{
    mat4 m4_View;
    mat4 m4_ModelView;
    mat4 m4_ModelViewProjection;
};

uniform Transform tr;

void main()
{
    gl_Position = tr.m4_ModelViewProjection * vec4(in_position,1.0);

    // Light in eye space
    vec3 LightPos = vec3(tr.m4_View * vec4(-1.0, 4.0, 2.0, 1.0));
    vec4 LightColor = vec4(0.9, 0.9, 1.0, 1.0);
    vec4 Ambient = vec4(0.05, 0.05, 0.05, 1.0);

    // Vertex and normal in Eye space
    vec3 modelViewVertex = vec3(tr.m4_ModelView * vec4(in_position,1.0));
    vec3 modelViewNormal = vec3(tr.m4_ModelView * vec4(in_normal, 0.0));

    // Lighting direction
    vec3 lightVector = normalize(LightPos - modelViewVertex);

    // Compute illumination (Gouraud)
    float distance = length(LightPos - modelViewVertex);
    float diffuse = max(dot(modelViewNormal, lightVector), 0.05);
    diffuse = diffuse * (1.0 / (1.0 + (0.05 * distance * distance)));


    vertex.Color    = LightColor * diffuse + Ambient;
    vertex.texCoord = in_texCoord;
}
