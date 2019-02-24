#version 400 core

layout (location = 0) in vec3 in_position;

out vec3 v3_texCoord;

uniform mat4 m4_view_projection;

void main()
{
    v3_texCoord = in_position;
    vec4 pos = m4_view_projection * vec4(in_position, 1.0);
    // z component always 1 after perspective divide
    gl_Position = pos.xyww;
}
