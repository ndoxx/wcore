#version 400 core

#include "convolution.glsl"

layout(location = 0) out vec4 out_color;

in vec2 texCoord;

struct GaussianKernel
{
    float f_weight[KERNEL_MAX_WEIGHTS];
    int i_half_size;
};

uniform sampler2D inputTex;
uniform bool horizontal;
uniform float f_alpha;
uniform vec2 v2_texelSize;
uniform GaussianKernel kernel;

#ifdef VARIANT_COMPRESS_R
    uniform float inv_gamma_r;
#endif

void main()
{
    #ifdef VARIANT_R_ONLY
    out_color.r = convolve_kernel_separable_r(kernel.f_weight, kernel.i_half_size,
                                              inputTex, texCoord,
                                              v2_texelSize, horizontal);
    #elif defined VARIANT_RGBA
    out_color = convolve_kernel_separable_rgba(kernel.f_weight, kernel.i_half_size,
                                               inputTex, texCoord,
                                               v2_texelSize, horizontal);
    #else
    out_color.rgb = convolve_kernel_separable(kernel.f_weight, kernel.i_half_size,
                                              inputTex, texCoord,
                                              v2_texelSize, horizontal);
    out_color.a = f_alpha;
    #endif

    #ifdef VARIANT_COMPRESS_R
        out_color.r = pow(out_color.r, inv_gamma_r);
    #endif
}
