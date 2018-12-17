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
    vec3 result = convolve_kernel_separable(kernel.f_weight, kernel.i_half_size,
                                            inputTex, texCoord,
                                            v2_texelSize, horizontal);
    #ifdef VARIANT_COMPRESS_R
        result.r = pow(result.r, inv_gamma_r);
    #endif
    out_color = vec4(result, f_alpha);
}
