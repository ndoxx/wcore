#version 400 core

#include "normal_compression.glsl"
#include "position.glsl"
#include "math_utils.glsl"

struct render_data
{
    vec2 v2_texelSize;

    // Position reconstruction
    vec4 v4_proj_params;
};
uniform render_data rd;

layout(location = 0) out vec4 out_SSR;

void main()
{
    out_SSR = vec4(0.0, 1.0, 0.0, 1.0);
}
