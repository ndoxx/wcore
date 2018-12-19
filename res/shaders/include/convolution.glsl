#define KERNEL_MAX_WEIGHTS 8

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

float convolve_kernel_separable_r(float weight[KERNEL_MAX_WEIGHTS], int half_size,
                                  sampler2D samp, vec2 texCoord, vec2 texOffset, bool horizontal)
{
    float result = texture(samp, texCoord).r * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int ii=1; ii<half_size; ++ii)
        {
            vec2 offset = vec2(texOffset.x * ii, 0.0f);
            result += texture(samp, texCoord + offset).r * weight[ii];
            result += texture(samp, texCoord - offset).r * weight[ii];
        }
    }
    else
    {
        for(int ii=1; ii<half_size; ++ii)
        {
            vec2 offset = vec2(0.0f, texOffset.y * ii);
            result += texture(samp, texCoord + offset).r * weight[ii];
            result += texture(samp, texCoord - offset).r * weight[ii];
        }
    }

    return result;
}
