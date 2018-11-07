#version 400 core

struct render_data
{
    float f_bright_threshold; // For bloom bright pass
    vec2 v2_screenSize;
};

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_bright_color;

// G-Buffer samplers
uniform sampler2D screenTex;

uniform render_data rd;

// CONSTANTS
// Relative luminance coefficients for sRGB primaries, values from Wikipedia
const vec3 W = vec3(0.2126, 0.7152, 0.0722);

void main()
{
    vec2 texCoord = gl_FragCoord.xy / rd.v2_screenSize;

    vec3 color = texture(screenTex, texCoord).xyz;

    // "Bright pass"
    float luminance = dot(color, W);
    //float brightnessMask = float(luminance > rd.f_bright_threshold); // Step function
    //float brightnessMask = 1/(1+exp(-20*(luminance-rd.f_bright_threshold))); // Sigmoid logistic function
    float brightnessMask = (1+tanh(20*(luminance-rd.f_bright_threshold)))/2; // Sigmoid hyperbolic tangent

    out_bright_color = brightnessMask*color;
}
