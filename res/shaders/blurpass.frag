#version 400 core

#include "gaussian_blur.glsl"

layout(location = 0) out vec4 out_color;

in vec2 texCoord;

uniform sampler2D inputTex;
uniform bool horizontal;
uniform float f_alpha;
uniform vec2 v2_texOffset;

void main()
{
    vec3 result = gaussian_blur_5(inputTex, texCoord, v2_texOffset, horizontal);
    out_color = vec4(result, f_alpha);
}
