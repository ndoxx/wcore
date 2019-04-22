#version 400 core

in vec2 texCoord;
layout(location = 0) out vec3 out_color;

// UNIFORMS
uniform sampler2D screenTex;
uniform bool b_isDepth;
uniform bool b_toneMap;
uniform bool b_splitAlpha;
uniform float f_splitPos = 0.5;

// Camera constants
uniform float f_near = 0.1;
uniform float f_far = 100.0;

void main()
{
    if(b_isDepth)
    {
        float depthNDC    = 2.0*texture(screenTex, texCoord).r - 1.0;
        float linearDepth = (2.0 * f_near /** f_far*/) / (f_far + f_near - depthNDC * (f_far - f_near));
        out_color = vec3(linearDepth,linearDepth,linearDepth);
    }
    else
    {
        // "screen" texture is a floating point color buffer
        vec4 sampleColor = texture(screenTex, texCoord);

        // Fast Reinhard tone mapping
        if(b_toneMap)
            sampleColor.rgb = sampleColor.rgb / (sampleColor.rgb + vec3(1.0));

        if(b_splitAlpha && texCoord.x>f_splitPos)
            sampleColor.rgb = vec3(sampleColor.a);

        out_color = sampleColor.rgb;
    }
}
