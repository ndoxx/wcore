#version 400 core

#include "normal_compression.glsl"
#include "shadow.glsl"
#include "cook_torrance.glsl"

struct render_data
{
    float f_wireframe_mix;    // Wireframe blend factor in[0,1]
    float f_bright_threshold; // For bloom bright pass
    vec3 v3_viewPos;
    vec2 v2_screenSize;
    // Shadow
    float f_shadowBias;
    vec2 v2_shadowTexelSize;
    bool b_shadow_enabled;
    // SSAO
    bool b_enableSSAO;
    // Lighting
    bool b_lighting_enabled;
#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
    // Position reconstruction
    vec4 v4_proj_params;
#endif
};

struct light
{
    vec3 v3_lightPosition;
    vec3 v3_lightColor;
    float f_light_radius;
    float f_ambientStrength;
};

//in vec4 fragLightSpace;
#ifdef VARIANT_DIRECTIONAL
    in vec2 frag_ray;
#endif

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_bright_color;

// G-Buffer samplers
#ifndef __EXPERIMENTAL_POS_RECONSTRUCTION__
uniform sampler2D positionTex;
#endif
uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D depthTex;
uniform sampler2D shadowTex;
uniform sampler2D SSAOTex;

uniform render_data rd;
uniform light       lt;
uniform mat4 m4_LightSpace;

// CONSTANTS
// Relative luminance coefficients for sRGB primaries, values from Wikipedia
const vec3 W = vec3(0.2126f, 0.7152f, 0.0722f);
// Bright pass step width
const float BP_STEP_WIDTH = 0.1f;

// Compute light attenuation for point lights
float attenuate(float distance, float radius, float compression=1.0f)
{
    //return clamp((1.0 -distance2/radius2),0.0,1.0)/(1.0 + distance2);
    //return clamp((-distance2*(1.0f/radius2) + 1.0f),0.0f,1.0f)/(1.0f + distance2);
    return pow(smoothstep(radius, 0, distance), compression);
}

vec3 reconstruct_position(in float depth, in vec2 ray, in vec4 projParams)
{
    float depth_ndc_offset = depth * 2.0f + projParams.z;
    float depth_view = projParams.w / depth_ndc_offset;
    return depth_view * vec3(ray, -1.0f);
}

void main()
{
    vec2 texCoord = gl_FragCoord.xy / rd.v2_screenSize;
    vec4 fNormMetAO = texture(normalTex, texCoord);

    vec3 fragNormal = decompress_normal(fNormMetAO.xy);
    float fragMetallic = fNormMetAO.b;
    float fragAO = fNormMetAO.a;

    #ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
        // Works, but introduces a little "black rectangle" glitch in the sky sometimes
        float depth     = texture(depthTex, texCoord).r;
        vec4 fAlbRough  = texture(albedoTex, texCoord);
        #ifdef VARIANT_DIRECTIONAL
            vec3 fragPos = reconstruct_position(depth, frag_ray, rd.v4_proj_params);
        #else
            vec2 ray = texCoord.xy*2.0f - 1.0f;
            ray *= rd.v4_proj_params.xy;
            vec3 fragPos = reconstruct_position(depth, ray, rd.v4_proj_params);
        #endif
        vec3 fragAlbedo = fAlbRough.rgb;
        float fragRoughness = fAlbRough.a;
    #else
        vec4 fPosRough  = texture(positionTex, texCoord);
        vec4 fAlbOv     = texture(albedoTex, texCoord);

        vec3 fragPos = fPosRough.xyz;
        float fragRoughness = fPosRough.a;
        float fragOverlay = fAlbOv.a;
        vec3 fragAlbedo = fAlbOv.rgb;
    #endif

    // View direction as a vector from fragment position to camera position
    vec3 viewDir = normalize(-fragPos);

    // SSAO
    if(rd.b_enableSSAO)
    {
        float fragSSAO = texture(SSAOTex, texCoord).r;
        fragAO *= fragSSAO;
    }

    vec3 total_light = vec3(0.0f);
    // Directional light
    #ifdef VARIANT_DIRECTIONAL
        float visibility = 1.0f;
        if(rd.b_shadow_enabled)
        {
            vec4 fragLightSpace = m4_LightSpace * vec4(fragPos, 1.0f);
            #ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
                visibility = shadow_variance(shadowTex, fragLightSpace);
            #else
                visibility = shadow_amount(shadowTex, fragLightSpace, rd.f_shadowBias, rd.v2_shadowTexelSize);
            #endif
        }
        if(rd.b_lighting_enabled)
        {
            total_light = CookTorrance(lt.v3_lightColor,
                                       lt.v3_lightPosition,
                                       fragNormal,
                                       viewDir,
                                       fragAlbedo,
                                       fragMetallic,
                                       fragRoughness,
                                       fragAO,
                                       visibility,
                                       lt.f_ambientStrength);
        }
        else
        {
            total_light = fragAlbedo*visibility;
        }
        #ifndef __EXPERIMENTAL_POS_RECONSTRUCTION__
            // Debug overlay & wireframe
            total_light = mix(total_light, vec3(0.5f), fragOverlay);
        #endif
    #endif

    // Point light
    #ifdef VARIANT_POINT
        vec3 diff = lt.v3_lightPosition - fragPos;
        vec3 normalizedLightDir = normalize(diff);
        float distance = length(diff);
        vec3 point_contrib = CookTorrance(lt.v3_lightColor,
                                          normalizedLightDir,
                                          fragNormal,
                                          viewDir,
                                          fragAlbedo,
                                          fragMetallic,
                                          fragRoughness,
                                          fragAO,
                                          1.0f,
                                          lt.f_ambientStrength);

        // attenuation
        total_light = point_contrib * attenuate(distance, lt.f_light_radius, 3.0f);
    #endif

    out_color = total_light;

    // "Bright pass"
    float luminance = dot(out_color, W);
    //float brightnessMask = float(luminance > rd.f_bright_threshold); // Step function
    //float brightnessMask = 1/(1+exp(-20*(luminance-rd.f_bright_threshold))); // Sigmoid logistic function
    //float brightnessMask = (1+tanh(30*(luminance-rd.f_bright_threshold)))/2; // Sigmoid hyperbolic tangent
    float brightnessMask = smoothstep(rd.f_bright_threshold-BP_STEP_WIDTH, rd.f_bright_threshold, luminance);

    out_bright_color = brightnessMask*out_color;
}
