#version 400 core

#include "normal_compression.glsl"
#include "shadow.glsl"
#include "cook_torrance.glsl"
#include "position.glsl"
#include "math_utils.glsl"

struct render_data
{
    float f_wireframe_mix;    // Wireframe blend factor in[0,1]
    float f_bright_threshold; // For bloom bright pass
    float f_bright_knee;      // For bloom bright pass
    vec3 v3_viewPos;
    vec2 v2_screenSize;
    // Shadow
    float f_shadowBias;
    vec2 v2_shadowTexelSize;
    bool b_shadow_enabled;
    // Ambient occlusion
    bool b_enableSSAO;
    // Reflections
    bool b_enableSSR;
    // Lighting
    bool b_lighting_enabled;
    // Position reconstruction
    vec4 v4_proj_params;
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
uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D depthTex;
uniform sampler2D shadowTex;
uniform sampler2D SSAOTex;
uniform sampler2D SSRTex;

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

vec3 brightness_prefilter(vec3 color, float threshold)
{
    float brightness = max(color.r, max(color.g, color.b));
    float contribution = max(0, brightness - threshold);
    contribution /= max(brightness, 0.00001);
    return color * contribution;
}

void main()
{
    vec2 texCoord = gl_FragCoord.xy / rd.v2_screenSize;
    vec4 fNormMetAO = texture(normalTex, texCoord);

    vec3 fragNormal = decompress_normal(fNormMetAO.xy);
    float fragMetallic = fNormMetAO.b;
    float fragAO = fNormMetAO.a;

    // Reconstruct position from depth buffer
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
            vec3 shadowMapCoords = fragLightSpace.xyz / fragLightSpace.w;

            // Calculate slope factor for slope-scaled depth bias
            float slopebias = clamp(dot(fragNormal,lt.v3_lightPosition),0.0f,1.0f);
            slopebias = sqrt(1.0f-slopebias*slopebias)/slopebias; // = tan(acos(slopebias));

            #ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
                visibility = shadow_variance(shadowTex, shadowMapCoords);
            #else
                //visibility = shadow_amount(shadowTex, shadowMapCoords, rd.f_shadowBias, rd.v2_shadowTexelSize);
                visibility = shadow_amount_Poisson(shadowTex, shadowMapCoords, fragPos, rd.f_shadowBias*slopebias, rd.v2_shadowTexelSize);
            #endif

            // Falloff around map edges
            float falloff = square_falloff(shadowMapCoords.xy, 0.5f);
            visibility = mix(1.0f, visibility, falloff);
        }
        if(rd.b_lighting_enabled)
        {
            vec3 radiance = CookTorrance(lt.v3_lightColor,
                                         lt.v3_lightPosition,
                                         fragNormal,
                                         viewDir,
                                         fragAlbedo,
                                         fragMetallic,
                                         fragRoughness,
                                         visibility);
            vec3 ambient = (fragAO * lt.f_ambientStrength) * fragAlbedo;
            total_light = radiance + ambient;
            if(rd.b_enableSSR)
            {
                vec4 reflection = texture(SSRTex, texCoord);
                total_light += reflection.rgb * reflection.a;
            }
        }
        else
        {
            total_light = fragAlbedo*visibility;
        }
    #endif

    // Point light
    #ifdef VARIANT_POINT
        vec3 diff = lt.v3_lightPosition - fragPos;
        vec3 normalizedLightDir = normalize(diff);
        float distance = length(diff);
        vec3 radiance = CookTorrance(lt.v3_lightColor,
                                     normalizedLightDir,
                                     fragNormal,
                                     viewDir,
                                     fragAlbedo,
                                     fragMetallic,
                                     fragRoughness,
                                     1.0f);
        vec3 ambient = (fragAO * lt.f_ambientStrength) * fragAlbedo;
        vec3 point_contrib = radiance + ambient;

        // attenuation
        total_light = point_contrib * attenuate(distance, lt.f_light_radius, 3.0f);
    #endif

    out_color = total_light;

    // "Bright pass"
    float luminance = dot(out_color, W);
    //float brightnessMask = float(luminance > rd.f_bright_threshold); // Step function
    //float brightnessMask = 1/(1+exp(-20*(luminance-rd.f_bright_threshold))); // Sigmoid logistic function
    //float brightnessMask = (1+tanh(30*(luminance-rd.f_bright_threshold)))/2; // Sigmoid hyperbolic tangent
    float brightnessMask = smoothstep(rd.f_bright_threshold-rd.f_bright_knee, rd.f_bright_threshold, luminance);
    //out_bright_color = brightness_prefilter(out_color, rd.f_bright_threshold);

    out_bright_color = brightnessMask*out_color;
}
