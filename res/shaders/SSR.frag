#version 400 core

#include "normal_compression.glsl"
#include "position.glsl"
#include "math_utils.glsl"

in vec2 frag_ray;

struct render_data
{
    vec2 v2_texelSize;

    mat4 m4_projection;
    mat4 m4_invView;
    float f_far;

    // Position reconstruction
    vec4 v4_proj_params;
};
uniform render_data rd;

uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D depthTex;
uniform sampler2D lastFrameTex;

layout(location = 0) out vec3 out_SSR;

const float step = 0.1;
const float minRayStep = 0.1;
const float maxSteps = 30;
const int numBinarySearchSteps = 10;
const float reflectionSpecularFalloffExponent = 3.0;

#define Scale vec3(.8, .8, .8)
#define K 19.19

vec3 hash(vec3 a);
vec2 binary_search(inout vec3 dir, inout vec3 hitCoord);
//vec2 ray_march(vec3 dir, inout vec3 hitCoord, out float dDepth);
vec2 ray_march(vec3 reflected, vec3 fragPos);

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

    // Reflection vector
    vec3 reflected = normalize(reflect(normalize(fragPos), fragNormal));

    //vec3 hitPos = fragPos;
    //float dDepth;

    /*vec3 wp = vec3(rd.m4_invView * vec4(fragPos, 1.0));
    vec3 jitt = mix(vec3(0.0), vec3(hash(wp)), fragRoughness);*/
    //vec2 coords = ray_march((/*vec3(jitt) +*/ reflected * max(minRayStep, -fragPos.z)), hitPos, dDepth);
    vec2 coords = ray_march(reflected, fragPos);

    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - coords.xy));
    float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);
    float ReflectionMultiplier = pow(fragMetallic, reflectionSpecularFalloffExponent) *
                screenEdgefactor *
                -reflected.z;

    out_SSR = texture2D(lastFrameTex, coords).rgb * clamp(ReflectionMultiplier, 0.0, 0.9);
    //out_SSR = texture2D(albedoTex, coords).rgb * clamp(ReflectionMultiplier, 0.0, 0.9);
}

vec2 binary_search(inout vec3 dir, inout vec3 hitCoord)
{
    float depth, dDepth;

    vec4 projectedCoord;

    for(int ii=0; ii<numBinarySearchSteps; ++ii)
    {

        projectedCoord = rd.m4_projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        depth = depth_view_from_tex(depthTex, projectedCoord.xy, rd.v4_proj_params.zw);

        dDepth = -hitCoord.z - depth;

        dir *= 0.5;
        hitCoord += (dDepth > 0.0) ? dir : -dir;
    }

    projectedCoord = rd.m4_projection * vec4(hitCoord, 1.0);
    projectedCoord.xy /= projectedCoord.w;
    projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

    return projectedCoord.xy;
}

vec2 ray_march(vec3 reflected, vec3 fragPos/*, float fragRoughness*/)
{
    /*vec3 wp = vec3(rd.m4_invView * vec4(fragPos, 1.0));
    vec3 jitt = mix(vec3(0.0), vec3(hash(wp)), fragRoughness);*/
    vec3 dir = step * (/*jitt +*/ reflected * max(minRayStep, -fragPos.z));
    vec3 hitCoord = fragPos;

    float depth;
    vec4 projectedCoord;

    for(int ii=0; ii<maxSteps; ++ii)
    {
        // March ray: advance hit point
        hitCoord += dir;
        // Project hit point to screen space
        projectedCoord = rd.m4_projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        // Get linear depth of nearest fragment at hit point from depth map
        depth = depth_view_from_tex(depthTex, projectedCoord.xy, rd.v4_proj_params.zw);

        /*if(depth > rd.f_far)
            continue;*/
        if(depth < -fragPos.z)
            continue;

        if(depth < -hitCoord.z)
        {
            //return projectedCoord.xy;
            return binary_search(dir, hitCoord);
        }
    }

    //return vec2(0.0);
    return projectedCoord.xy;
    //discard;
}

vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}
