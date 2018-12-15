#define KERNEL_MAX_WEIGHTS 8

const float GB_WEIGHTS9[5] = float[] (0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f);

//const float GB_WEIGHTS9[5] = float[] (0.382928f, 0.241732f, 0.060598f, 0.005977f, 0.000229f);


vec3 gaussian_blur_9_rgb(sampler2D samp, vec2 texCoord, vec2 texOffset, bool horizontal)
{
    vec3 result = texture(samp, texCoord).rgb * GB_WEIGHTS9[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(texOffset.x * ii, 0.0f);
            result += texture(samp, texCoord + offset).rgb * GB_WEIGHTS9[ii];
            result += texture(samp, texCoord - offset).rgb * GB_WEIGHTS9[ii];
        }
    }
    else
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(0.0f, texOffset.y * ii);
            result += texture(samp, texCoord + offset).rgb * GB_WEIGHTS9[ii];
            result += texture(samp, texCoord - offset).rgb * GB_WEIGHTS9[ii];
        }
    }

    return result;
}

vec4 gaussian_blur_9_rgba(sampler2D samp, vec2 texCoord, vec2 texOffset, bool horizontal)
{
    vec4 result = texture(samp, texCoord) * GB_WEIGHTS9[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(texOffset.x * ii, 0.0f);
            result += texture(samp, texCoord + offset) * GB_WEIGHTS9[ii];
            result += texture(samp, texCoord - offset) * GB_WEIGHTS9[ii];
        }
    }
    else
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(0.0f, texOffset.y * ii);
            result += texture(samp, texCoord + offset) * GB_WEIGHTS9[ii];
            result += texture(samp, texCoord - offset) * GB_WEIGHTS9[ii];
        }
    }

    return result;
}

vec3 convolve_kernel_separable(float weight[KERNEL_MAX_WEIGHTS], int half_size,
                               sampler2D samp, vec2 texCoord, vec2 texOffset, bool horizontal)
{
    vec3 result = texture(samp, texCoord).rgb * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int ii=1; ii<half_size; ++ii)
        {
            vec2 offset = vec2(texOffset.x * ii, 0.0f);
            result += texture(samp, texCoord + offset).rgb * weight[ii];
            result += texture(samp, texCoord - offset).rgb * weight[ii];
        }
    }
    else
    {
        for(int ii=1; ii<half_size; ++ii)
        {
            vec2 offset = vec2(0.0f, texOffset.y * ii);
            result += texture(samp, texCoord + offset).rgb * weight[ii];
            result += texture(samp, texCoord - offset).rgb * weight[ii];
        }
    }

    return result;
}
