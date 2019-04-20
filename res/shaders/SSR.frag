#version 400 core

#include "normal_compression.glsl"
#include "position.glsl"
#include "math_utils.glsl"
#include "cook_torrance.glsl"


in vec2 frag_ray;

struct render_data
{
    vec2 v2_viewportSize;
    vec2 v2_texelSize;

    mat4 m4_projection;
    mat4 m4_invView;
    float f_near;
    float f_pixelThickness;
    float f_maxRayDistance;
    float f_pixelStride;         // number of pixels per ray step close to camera
    float f_pixelStrideZCuttoff; // ray origin Z at this distance will have a pixel stride of 1.0
    float f_iterations;
    float f_binSearchIterations;
    float f_screenEdgeFadeStart; // distance to screen edge that ray hits will start to fade (0.0 -> 1.0)
    float f_eyeFadeStart;        // ray direction's Z that ray hits will start to fade (0.0 -> 1.0)
    float f_eyeFadeEnd;          // ray direction's Z that ray hits will be cut (0.0 -> 1.0)
    float f_jitterAmount;

    // Position reconstruction
    vec4 v4_proj_params;
};
uniform render_data rd;

uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D depthTex;
uniform sampler2D lastFrameTex;
//uniform sampler2D backDepthTex;

layout(location = 0) out vec4 out_SSR;


void swap_if_bigger(inout float aa, inout float bb)
{
    if(aa > bb)
    {
        float tmp = aa;
        aa = bb;
        bb = tmp;
    }
}

bool ray_intersects_depth_buffer(float rayZNear, float rayZFar, vec2 hitPixel)
{
    // Swap if bigger
    if(rayZFar > rayZNear)
    {
        float t = rayZFar; rayZFar = rayZNear; rayZNear = t;
    }
    float cameraZ = -depth_view_from_tex(depthTex, hitPixel.xy, rd.v4_proj_params.zw);
    // Cross z
    return rayZFar <= cameraZ && rayZNear >= cameraZ - rd.f_pixelThickness;
}
/*
bool ray_intersects_depth_buffer(float rayZNear, float rayZFar, vec2 hitPixel)
{
    // Swap if bigger
    if(rayZFar > rayZNear)
    {
        float t = rayZFar; rayZFar = rayZNear; rayZNear = t;
    }
    float cameraZ = -depth_view_from_tex(depthTex, hitPixel.xy, rd.v4_proj_params.zw);
    float backZ = -depth_view_from_tex(backDepthTex, hitPixel.xy, rd.v4_proj_params.zw);
    // Cross z
    return rayZFar <= cameraZ && rayZNear >= backZ - rd.f_pixelThickness;
}*/

bool ray_march(vec3 rayOrigin,
               vec3 rayDirection,
               float jitter,
               out vec2 hitPixel,
               out vec3 hitPoint,
               out float iterationCount)
{
    // Clip to the near plane
    float rayLength = ((rayOrigin.z + rayDirection.z * rd.f_maxRayDistance) > -rd.f_near) ?
                      (-rd.f_near - rayOrigin.z) / rayDirection.z : rd.f_maxRayDistance;
    vec3 rayEnd = rayOrigin + rayDirection * rayLength;

    // Project into homogeneous clip space
    vec4 H0 = rd.m4_projection * vec4(rayOrigin, 1.0);
    vec4 H1 = rd.m4_projection * vec4(rayEnd, 1.0);

    float k0 = 1.f / H0.w, k1 = 1.f / H1.w;

    // The interpolated homogeneous version of the camera-space points
    vec3 Q0 = rayOrigin * k0, Q1 = rayEnd * k1;

    // Screen-space endpoints
    //vec2 P0 = H0.xy * k0, P1 = H1.xy * k1;
    vec2 P0 = (H0.xy * k0 * 0.5 + 0.5) * rd.v2_viewportSize;
    vec2 P1 = (H1.xy * k1 * 0.5 + 0.5) * rd.v2_viewportSize;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += dot(P1 - P0, P1 - P0) < 0.0001f ? 0.01f : 0.0f;

    vec2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to collapse
    // all quadrant-specific DDA cases later
    bool permute = false;
    if (abs(delta.x) < abs(delta.y)) {
        // This is a more-vertical line
        permute = true; delta = delta.yx; P0 = P0.yx; P1 = P1.yx;
    }

    float stepDir = sign(delta.x);
    float invdx = stepDir / delta.x;

    // Track the derivatives of Q and k
    vec3  dQ = (Q1 - Q0) * invdx;
    float dk = (k1 - k0) * invdx;
    vec2  dP = vec2(stepDir, delta.y * invdx);

    // Calculate pixel stride based on distance of ray origin from camera.
    // Since perspective means distant objects will be smaller in screen space
    // we can use this to have higher quality reflections for far away objects
    // while still using a large pixel stride for near objects (and increase performance)
    // this also helps mitigate artifacts on distant reflections when we use a large
    // pixel stride.
    float strideScaler = 1.f - min(1.f, -rayOrigin.z / rd.f_pixelStrideZCuttoff);
    float pixelStride = 1.f + strideScaler * rd.f_pixelStride;

    // Scale derivatives by the desired pixel stride and then
    // offset the starting values by the jitter fraction
    dP *= pixelStride; dQ *= pixelStride; dk *= pixelStride;


    // Track ray step and derivatives in a vec4 to parallelize
    vec4 pqk = vec4(P0, Q0.z, k0);
    vec4 dPQK = vec4(dP, dQ.z, dk);

    pqk += dPQK * jitter;
    float rayZFar = (dPQK.z * 0.5 + pqk.z) / (dPQK.w * 0.5 + pqk.w);
    float rayZNear;
    bool intersect = false;
    float ii;

    for(ii=0.f; ii<rd.f_iterations && !intersect; ++ii)
    {
        pqk += dPQK;

        rayZNear = rayZFar;
        rayZFar = (dPQK.z * 0.5f + pqk.z) / (dPQK.w * 0.5f + pqk.w);

        hitPixel = permute ? pqk.yx : pqk.xy;
        hitPixel *= rd.v2_texelSize;

        intersect = ray_intersects_depth_buffer(rayZNear, rayZFar, hitPixel);
    }

    // Binary search refinement
    if(pixelStride > 1.f && intersect)
    {
        pqk -= dPQK;
        dPQK /= pixelStride;

        float originalStride = pixelStride * 0.5f;
        float stride = originalStride;

        for(float jj=0; jj<rd.f_binSearchIterations; ++jj)
        {
            pqk += dPQK * stride;

            rayZNear = rayZFar;
            rayZFar = (dPQK.z * -0.5f + pqk.z) / (dPQK.w * -0.5f + pqk.w);

            hitPixel = permute ? pqk.yx : pqk.xy;
            hitPixel *= rd.v2_texelSize;

            originalStride *= 0.5f;
            stride = ray_intersects_depth_buffer(rayZNear, rayZFar, hitPixel) ? -originalStride : originalStride;
        }
    }


    Q0.xy += dQ.xy * ii;
    Q0.z = pqk.z;
    hitPoint = Q0 / pqk.w;
    iterationCount = ii;

    return intersect;
}

float ssr_attenuation(bool intersect,
                      float iterationCount,
                      float reflectivity,
                      vec2 hitPixel,
                      vec3 hitPoint,
                      vec3 vsRayOrigin,
                      vec3 vsRayDirection)
{
    float alpha = clamp(reflectivity, 0.f, 1.f);

    // Fade ray hits that approach the maximum iterations
    alpha *= 1.f - (iterationCount / rd.f_iterations);

    // Fade ray hits that approach the screen edge
    float screenFade = rd.f_screenEdgeFadeStart;
    vec2 hitPixelNDC = (hitPixel * 2.f - 1.f);
    float maxDimension = min(1.f, max(abs(hitPixelNDC.x), abs(hitPixelNDC.y)));
    alpha *= 1.f - (max(0.f, maxDimension - screenFade) / (1.f - screenFade));

    // Fade ray hits based on how much they face the camera
    float eyeFadeStart = rd.f_eyeFadeStart;
    float eyeFadeEnd = rd.f_eyeFadeEnd;
    swap_if_bigger(eyeFadeStart, eyeFadeEnd);

    float eyeDirection = clamp(vsRayDirection.z, eyeFadeStart, eyeFadeEnd);
    alpha *= 1.f - ((eyeDirection - eyeFadeStart) / (eyeFadeEnd - eyeFadeStart));

    // Fade ray hits based on distance from ray origin
    alpha *= 1.f - clamp(distance(vsRayOrigin, hitPoint) / rd.f_maxRayDistance, 0.f, 1.f);

    alpha *= float(intersect);

    return alpha;
}

#define Scale vec3(.8, .8, .8)
#define K 19.19
vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}

void main()
{
    vec2 texCoord = gl_FragCoord.xy * rd.v2_texelSize;

    vec4 fNormMetAO = texture(normalTex, texCoord);
    float fragMetallic = fNormMetAO.b;
    if(fragMetallic < 0.01)
        discard;

    vec3 fragNormal = normalize(decompress_normal(fNormMetAO.xy));

    float depth = texture(depthTex, texCoord).r;
    vec3 vsRayOrigin = reconstruct_position(depth, frag_ray, rd.v4_proj_params);

    vec4 fAlbRough = texture(albedoTex, texCoord);
    float fragRoughness = fAlbRough.a;
    float specularStrength = 1.f - fragRoughness;

    // Fresnel coefficient vector
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, fAlbRough.rgb, fragMetallic);
    vec4 fresnel = vec4(FresnelGS(max(dot(fragNormal, normalize(vsRayOrigin)), 0.f), F0), 1.f);

    vec3 vsRayDirection = normalize(reflect(normalize(vsRayOrigin), fragNormal));

    vec2 hitPixel;
    vec3 hitPoint;
    float iterationCount;

    float c = (gl_FragCoord.x + gl_FragCoord.y) * 0.25f;
    float jitter = mod(c, 1.f);

    /*vec3 wp = vec3(rd.m4_invView * vec4(vsRayOrigin, 1.0));
    vec3 jitt = rd.f_jitterAmount * mix(vec3(0.0), vec3(hash(wp)), fragRoughness);
    vsRayDirection = normalize(vsRayDirection + jitt);*/

    bool intersect = ray_march(vsRayOrigin, vsRayDirection, jitter, hitPixel, hitPoint, iterationCount/*, texCoord.x > 0.5*/);
    float alpha = ssr_attenuation(intersect, iterationCount, specularStrength, hitPixel, hitPoint, vsRayOrigin, vsRayDirection);

    out_SSR = (intersect ? vec4(texture(lastFrameTex, hitPixel).rgb, alpha) : vec4(0.f)) * fresnel;
}
