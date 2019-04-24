#version 400 core

#include "position.glsl"

in vec2 texCoord;
layout(location = 0) out vec3 out_color;

// UNIFORMS
uniform sampler2D screenTex;
uniform bool b_isDepth;
uniform bool b_toneMap = true;
uniform bool b_splitAlpha = false;
uniform bool b_invert = false;
uniform float f_splitPos = 0.5;
uniform vec3 v3_channelFilter = vec3(1.0);
uniform vec4 v4_proj_params;
uniform vec2 v2_texelSize;

void main()
{
    if(b_isDepth)
    {
        float linearDepth = depth_view_from_tex(screenTex, texCoord, v4_proj_params.zw);
        float depthRemapped = linearDepth / (linearDepth + 1.0);
        out_color = vec3(depthRemapped,depthRemapped,depthRemapped);
    }
    else
    {
        // "screen" texture is a floating point color buffer
        vec4 sampleColor = texture(screenTex, texCoord);

        // Filter channels
        sampleColor.rgb *= v3_channelFilter;

        // If single channel, display in gray levels
        if(dot(v3_channelFilter, vec3(1.f))<2.f)
        {
            float monochrome = dot(sampleColor.rgb, v3_channelFilter);
            sampleColor.rgb = vec3(monochrome);
        }

        // Fast Reinhard tone mapping
        sampleColor.rgb = b_toneMap ? sampleColor.rgb / (sampleColor.rgb + vec3(1.0)) : sampleColor.rgb;

        // Color inversion
        sampleColor.rgb = b_invert ? 1.f - sampleColor.rgb : sampleColor.rgb;

        if(b_splitAlpha && texCoord.x>f_splitPos)
        {
            if(texCoord.x<f_splitPos+1.01f*v2_texelSize.x)
                sampleColor.rgb = vec3(1.0,0.5,0.0);
            else
                sampleColor.rgb = vec3(sampleColor.a);
        }

        out_color = sampleColor.rgb;
    }
}
