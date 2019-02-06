#version 400 core

in vec2 texCoord;
layout(location = 0) out vec3 out_color;

// UNIFORMS
uniform sampler2D screenTex;
uniform bool b_isDepth;

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
        vec3 hdrColor = texture(screenTex, texCoord).rgb;
        // Fast Reinhard tone mapping
        out_color = hdrColor / (hdrColor + vec3(1.0));
    }
}
