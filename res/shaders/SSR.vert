#version 400 core

struct render_data
{
    vec2 v2_texelSize;

    // Position reconstruction
    vec4 v4_proj_params;
};
uniform render_data rd;

layout(location = 0) in vec3 in_position;

out vec2 frag_ray;

void main()
{
    vec4 clip_pos = vec4(in_position, 1.0);
    frag_ray = clip_pos.xy*rd.v4_proj_params.xy;
    gl_Position = clip_pos;
}
