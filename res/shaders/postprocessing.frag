#version 400 core

in vec2 texCoord;
layout(location = 0) out vec3 out_color;

// STRUCTURES:
struct Renderer
{
    //PP
    vec3 v3_gamma;
    float f_saturation;
    float f_exposure;
    //FXAA
    vec2 v2_frameBufSize;
    bool b_FXAA_enabled;
    //LightScatt
    vec2 v2_lightScatterSourcePos;
    //Fog
    vec3 v3_fogColor;
    float f_fogDensity;
    bool b_enableFog;
    //Bloom
    bool b_enableBloom;
};

// Relative luminance coefficients for sRGB primaries, values from Wikipedia
const vec3 W = vec3(0.2126f, 0.7152f, 0.0722f);

// FXAA constants
const float FXAA_SPAN_MAX = 8.0f;
const float FXAA_REDUCE_MUL = 1.0f/8.0f;
const float FXAA_REDUCE_MIN = 1.0f/128.0f;
const float FXAA_EDGE_THRESHOLD = 1.0f/8.0f;      // 1/4 low-Q 1/8 high-Q
const float FXAA_EDGE_THRESHOLD_MIN = 1.0f/32.0f; // 1/12 upper limit 1/32 visible limit
const float LUMA_COEFF = 0.587f/0.299f;
const vec3  LUMA = vec3(0.299f, 0.587f, 0.114f);

// Camera constants
const float NEAR = 0.1f;
const float FAR = 100.0f;

#define ONE_THIRD 0.33333333f
#define TWO_THIRD 0.66666666f
#define ONE_THIRD_MIN_05 -0.16666667f
#define TWO_THIRD_MIN_05 0.16666667f

// UNIFORMS
uniform sampler2D screenTex;
uniform sampler2D bloomTex;
uniform sampler2D depthStencilTex;
uniform sampler2D SSAOTex;
uniform Renderer rd;

// FUNCTIONS:
vec3 saturate(vec3 color_in, float adjustment)
{
    vec3 luminance = vec3(dot(color_in, W));
    return mix(luminance, color_in, adjustment);
}

vec3 gamma_correct(vec3 color_in, vec3 gamma_factor)
{
    return pow(color_in, 1.0/gamma_factor);
}

vec3 FXAA(sampler2D samp, vec2 texCoords);
vec3 light_scattering(vec2 texCoord, vec2 screenLightPos, float density, float weight, float decay, float exposure);

void main()
{
    vec3 hdrColor;
    // Fast Approximate Anti-Aliasing
    if(rd.b_FXAA_enabled)
        hdrColor = FXAA(screenTex, texCoord);
    else
        hdrColor = texture(screenTex, texCoord).rgb;

    // texture for bloom effect
    if(rd.b_enableBloom)
    {
        vec3 bloom_color = texture(bloomTex, texCoord).rgb;
        hdrColor += bloom_color;
    }

    // Exposure tone mapping
    vec3 mapped = vec3(1.0f) - exp(-hdrColor * rd.f_exposure);

    // Fog
    if(rd.b_enableFog)
    {
        float depthNDC    = 2.0f*texture(depthStencilTex, texCoord).r - 1.0f;
        float linearDepth = (2.0f * NEAR * FAR) / ((FAR + NEAR) - depthNDC * (FAR - NEAR));
        float fogFactor   = 1.0f/exp(pow((linearDepth * rd.f_fogDensity),3));
        fogFactor         = clamp(fogFactor, 0.0f, 1.0f);
        mapped            = mix(rd.v3_fogColor, mapped, fogFactor);
    }

    // Color saturation
    mapped = saturate(mapped, rd.f_saturation);

    // Gamma correction
    out_color = gamma_correct(mapped, rd.v3_gamma);
}

// Luminance calculated on R and G channels only (faster)
float FxaaLuma(vec3 color)
{
    return color.g * LUMA_COEFF + color.r;
}

vec3 FXAA(sampler2D samp, vec2 texCoords)
{
    // Get diagonal neighbors
    vec3 rgbNW = texture(samp, texCoords+(vec2(-1.0f,-1.0f)/rd.v2_frameBufSize)).rgb;
    vec3 rgbNE = texture(samp, texCoords+(vec2(1.0f,-1.0f)/rd.v2_frameBufSize)).rgb;
    vec3 rgbSW = texture(samp, texCoords+(vec2(-1.0f,1.0f)/rd.v2_frameBufSize)).rgb;
    vec3 rgbSE = texture(samp, texCoords+(vec2(1.0f,1.0f)/rd.v2_frameBufSize)).rgb;
    vec3 rgbM  = texture(samp, texCoords).rgb;

    // Convert to luminance
    float lumaNW = FxaaLuma(rgbNW); //dot(rgbNW, LUMA);
    float lumaNE = FxaaLuma(rgbNE); //dot(rgbNE, LUMA);
    float lumaSW = FxaaLuma(rgbSW); //dot(rgbSW, LUMA);
    float lumaSE = FxaaLuma(rgbSE); //dot(rgbSE, LUMA);
    float lumaM  = FxaaLuma(rgbM);  //dot(rgbM,  LUMA);

    // Find brightest / darkest neighbor
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // Local contrast check
    // Early exit if luminance range within neighborhood is small enough
    float range = lumaMax-lumaMin;
    if(range < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD))
    {
        return rgbM;
    }

    // Edge detection, filtering
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25f * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0f/(min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) / rd.v2_frameBufSize;

    vec3 rgbA = 0.5f * (
        texture(samp, ONE_THIRD_MIN_05 * dir + texCoords.xy).xyz +
        texture(samp, TWO_THIRD_MIN_05 * dir + texCoords.xy).xyz);
    vec3 rgbB = (rgbA * 0.5f) + (0.25f * (
        texture(samp, -0.5f * dir + texCoords.xy).xyz +
        texture(samp,  0.5f * dir + texCoords.xy).xyz));
    float lumaB = dot(rgbB, LUMA);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return rgbA;
    }
    return rgbB;
}

/*
vec3 light_scattering(vec2 texCoord, vec2 screenLightPos, float density, float weight, float decay, float exposure)
{
    // Calculate vector from pixel to light source in screen space.
    vec2 deltaTexCoord = (texCoord - screenLightPos.xy);
    // Divide by number of samples and scale by control factor.
    deltaTexCoord *= LS_NUM_SAMPLES_INV * density;
    // Store initial sample.
    //vec3 color = texture(screenTex, texCoord);
    vec3 color = vec3(0.0,0.0,0.0);
    // Set up illumination decay factor.
    float illuminationDecay = 1.0f;
    // Evaluate summation from Equation 3 LS_NUM_SAMPLES iterations.
    for (int ii=0; ii<LS_NUM_SAMPLES; ++ii)
    {
        // Step sample location along ray.
        texCoord -= deltaTexCoord;
        // Retrieve sample at new location.
        vec3 samp = texture(bloomTex, texCoord).rgb;
        // Apply sample attenuation scale/decay factors.
        samp *= illuminationDecay * weight;
        // Accumulate combined color.
        color += samp;
        // Update exponential decay factor.
        illuminationDecay *= decay;
    }
    // Output final color with a further scale control factor.
    return color * exposure;
}*/
