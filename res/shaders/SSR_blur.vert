#version 400 core

struct render_data
{
    vec2 v2_texelSize;
    vec2 v2_texelOffsetScale;
    vec4 v4_proj_params;

    float f_depthBias;
    float f_normalBias;
    float f_blurQuality;

    int i_samples;
};
uniform render_data rd;

layout(location = 0) in vec3 in_position;

void main()
{
    gl_Position = vec4(in_position, 1.0);
}
