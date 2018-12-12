const float GB_WEIGHTS5[5] = float[] (0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f);

vec3 gaussian_blur_5_rgb(sampler2D samp, vec2 texCoord, vec2 texOffset, bool horizontal)
{
    vec3 result = texture(samp, texCoord).rgb * GB_WEIGHTS5[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(texOffset.x * ii, 0.0f);
            result += texture(samp, texCoord + offset).rgb * GB_WEIGHTS5[ii];
            result += texture(samp, texCoord - offset).rgb * GB_WEIGHTS5[ii];
        }
    }
    else
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(0.0f, texOffset.y * ii);
            result += texture(samp, texCoord + offset).rgb * GB_WEIGHTS5[ii];
            result += texture(samp, texCoord - offset).rgb * GB_WEIGHTS5[ii];
        }
    }

    return result;
}

vec4 gaussian_blur_5_rgba(sampler2D samp, vec2 texCoord, vec2 texOffset, bool horizontal)
{
    vec4 result = texture(samp, texCoord) * GB_WEIGHTS5[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(texOffset.x * ii, 0.0f);
            result += texture(samp, texCoord + offset) * GB_WEIGHTS5[ii];
            result += texture(samp, texCoord - offset) * GB_WEIGHTS5[ii];
        }
    }
    else
    {
        for(int ii=1; ii<5; ++ii)
        {
            vec2 offset = vec2(0.0f, texOffset.y * ii);
            result += texture(samp, texCoord + offset) * GB_WEIGHTS5[ii];
            result += texture(samp, texCoord - offset) * GB_WEIGHTS5[ii];
        }
    }

    return result;
}
