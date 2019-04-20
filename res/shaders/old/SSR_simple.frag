#version 400 core

#include "normal_compression.glsl"
#include "position.glsl"
#include "math_utils.glsl"
#include "cook_torrance.glsl"

in vec2 frag_ray;

struct render_data
{
    vec2 v2_texelSize;

    mat4 m4_projection;
    mat4 m4_invView;
    float f_far;
    float f_hitThreshold;
    float f_step;
    float f_reflectionFalloff;
    float f_jitterAmount;
    int i_raySteps;
    int i_binSteps;

    // Position reconstruction
    vec4 v4_proj_params;
};
uniform render_data rd;

uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D depthTex;
uniform sampler2D lastFrameTex;

layout(location = 0) out vec3 out_SSR;

const float minRayStep = 0.1;

#define Scale vec3(.8, .8, .8)
#define K 19.19

vec3 hash(vec3 a);
vec4 binary_search(inout vec3 dir, inout vec3 hitCoord);
vec4 ray_march(vec3 reflected, vec3 fragPos, float fragRoughness);

void main()
{
    vec2 texCoord = gl_FragCoord.xy * rd.v2_texelSize;

    vec4 fNormMetAO = texture(normalTex, texCoord);
    float fragMetallic = fNormMetAO.b;
    if(fragMetallic < 0.01)
        discard;

    vec3 fragNormal = normalize(decompress_normal(fNormMetAO.xy));

    float depth     = texture(depthTex, texCoord).r;
    vec3 fragPos = reconstruct_position(depth, frag_ray, rd.v4_proj_params);

    vec4 fAlbRough  = texture(albedoTex, texCoord);
    float fragRoughness = fAlbRough.a;

    // Fresnel coefficient vector
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, fAlbRough.rgb, fragMetallic);
    vec3 fresnel = FresnelGS(max(dot(fragNormal, normalize(fragPos)), 0.0), F0);

    // Reflection vector
    vec3 reflected = normalize(reflect(fragPos, fragNormal));

    vec4 coords = ray_march(reflected, fragPos, fragRoughness);

    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - coords.xy));
    float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);
    float ReflectionMultiplier = pow(fragMetallic, rd.f_reflectionFalloff)
                                 * screenEdgefactor
                                 * clamp(-reflected.z, 0.f, 1.f)
                                 * coords.w;

    out_SSR = texture2D(lastFrameTex, coords.xy).rgb * clamp(ReflectionMultiplier, 0.0, 1.0) * fresnel;
}

vec4 binary_search(inout vec3 dir, inout vec3 hitCoord)
{
    float depth, dDepth;

    vec4 projectedCoord;

    for(int ii=0; ii<rd.i_binSteps; ++ii)
    {

        projectedCoord = rd.m4_projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        depth = depth_view_from_tex(depthTex, projectedCoord.xy, rd.v4_proj_params.zw);

        dDepth = -hitCoord.z - depth;

        dir *= 0.5;
        hitCoord += (dDepth > 0.0) ? -dir : +dir;
    }

    projectedCoord = rd.m4_projection * vec4(hitCoord, 1.0);
    projectedCoord.xy /= projectedCoord.w;
    projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

    //return vec4(projectedCoord.xy, depth, (abs(dDepth)<rd.f_hitThreshold) ? 1.f : 0.f);
    return vec4(projectedCoord.xy, depth, clamp(rd.f_hitThreshold-abs(dDepth),0.f,1.f));
}

vec4 ray_march(vec3 reflected, vec3 fragPos, float fragRoughness)
{
    vec3 wp = vec3(rd.m4_invView * vec4(fragPos, 1.0));
    vec3 jitt = rd.f_jitterAmount * mix(vec3(0.0), vec3(hash(wp)), fragRoughness);
    vec3 dir = jitt + reflected /* max(minRayStep, -fragPos.z)*/;
    vec3 hitCoord = fragPos;

    float depth, dDepth;
    vec4 projectedCoord;

    for(int ii=0; ii<rd.i_raySteps; ++ii)
    {
        // March ray: advance hit point
        hitCoord += dir;
        // Project hit point to screen space
        projectedCoord = rd.m4_projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        // Get linear depth of nearest fragment at hit point from depth map
        depth = depth_view_from_tex(depthTex, projectedCoord.xy, rd.v4_proj_params.zw);

        dDepth = -hitCoord.z - depth;
        if(dDepth > 0.f)
            return binary_search(dir, hitCoord);

        dir *= rd.f_step;
    }

    return vec4(0.f);
    //return vec4(projectedCoord.xy, depth, (abs(dDepth)<rd.f_hitThreshold) ? 1.f : 0.f);
}

vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}
