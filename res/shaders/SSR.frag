#version 400 core
#pragma hotswap

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
    mat4 m4_reproj;
    float f_near;
    float f_minGlossiness;
    //float f_pixelThickness;
    float f_maxRayDistance;
    float f_pixelStride;         // number of pixels per ray step close to camera
    float f_pixelStrideZCuttoff; // ray origin Z at this distance will have a pixel stride of 1.0
    float f_iterations;
    float f_binSearchIterations;
    float f_screenEdgeFadeStart; // distance to screen edge that ray hits will start to fade (0.0 -> 1.0)
    float f_eyeFadeStart;        // ray direction's Z that ray hits will start to fade (0.0 -> 1.0)
    float f_eyeFadeEnd;          // ray direction's Z that ray hits will be cut (0.0 -> 1.0)
    float f_ditherAmount;

    float f_probe; // dbg

    // Position reconstruction
    vec4 v4_proj_params;
};
uniform render_data rd;

uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D depthTex;
uniform sampler2D backDepthTex;
uniform sampler2D lastFrameTex;

layout(location = 0) out vec4 out_SSR;


void swap_if_bigger(inout float aa, inout float bb)
{
    /*if(aa > bb)
    {
        float tmp = aa;
        aa = bb;
        bb = tmp;
    }*/
    bool bigger = aa > bb;
    float tmp = aa;
    aa = bigger ? bb : aa;
    bb = bigger ? tmp: bb;
}

/*bool ray_intersects_depth_buffer(float rayZNear, float rayZFar, vec2 hitPixel)
{
    swap_if_bigger(rayZFar, rayZNear);
    float cameraZ = -depth_view_from_tex(depthTex, hitPixel.xy, rd.v4_proj_params.zw);
    // Cross z
    return rayZFar <= cameraZ && rayZNear >= cameraZ - rd.f_pixelThickness;
}*/


bool ray_intersects_depth_buffer(float rayZNear, float rayZFar, vec2 hitPixel)
{
    // Swap if bigger
    swap_if_bigger(rayZFar, rayZNear);
    float cameraZ = -depth_view_from_tex(depthTex, hitPixel.xy, rd.v4_proj_params.zw);
    float backZ = -depth_view_from_tex(backDepthTex, hitPixel.xy, rd.v4_proj_params.zw);
    // Cross z
    return rayZFar <= cameraZ && rayZNear >= backZ;
}

bool ray_march(vec3 rayOrigin,
               vec3 rayDirection,
               float jitter,
               out vec2 hitPixel,
               out vec3 hitPoint,
               out float iterationCount)
{
    // Clip to the near plane
    float rayLength = ((rayDirection.z * rd.f_maxRayDistance + rayOrigin.z) > -rd.f_near) ?
                      (-rd.f_near - rayOrigin.z) / rayDirection.z : rd.f_maxRayDistance;
    vec3 rayEnd = rayDirection * rayLength + rayOrigin;

    // Project into homogeneous clip space
    vec4 H0 = rd.m4_projection * vec4(rayOrigin, 1.0);
    vec4 H1 = rd.m4_projection * vec4(rayEnd, 1.0);

    float k0 = 1.f / H0.w, k1 = 1.f / H1.w;

    // The interpolated homogeneous version of the camera-space points
    vec3 Q0 = rayOrigin * k0, Q1 = rayEnd * k1;

    // Screen-space endpoints
    vec2 P0 = ((H0.xy * k0) * 0.5f + 0.5f) * rd.v2_viewportSize;
    vec2 P1 = ((H1.xy * k1) * 0.5f + 0.5f) * rd.v2_viewportSize;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    vec2 delta = P1 - P0;
    float p_corr = dot(delta, delta) < 0.0001f ? 0.01f : 0.0f;
    P1 += p_corr;
    delta += p_corr;

    // Permute so that the primary iteration is in x to collapse
    // all quadrant-specific DDA cases later
    bool permute = false;
    if(abs(delta.x) < abs(delta.y))
    {
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
    float strideScaler = 1.f - min(1.f, -rayOrigin.z * rd.f_pixelStrideZCuttoff);
    float pixelStride = strideScaler * rd.f_pixelStride + 1.f;

    // Track ray step and derivatives in a vec4 to parallelize
    vec4 pqk = vec4(P0, Q0.z, k0);
    vec4 dPQK = vec4(dP, dQ.z, dk);

    // Scale derivatives by the desired pixel stride and then
    // offset the starting values by the jitter fraction
    dPQK *= pixelStride;
    pqk += dPQK * jitter;

    float rayZFar = (dPQK.z * 0.5f + pqk.z) / (dPQK.w * 0.5f + pqk.w);
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
    if(pixelStride>1.f && intersect)
    {
        pqk -= dPQK; // step back
        dPQK /= pixelStride; // unscale derivatives

        float stride = pixelStride * 0.5f;
        vec4 dPQKj;

        for(float jj=0; jj<rd.f_binSearchIterations && stride>0.5f; ++jj)
        {
            dPQKj = dPQK * stride;
            pqk += dPQKj;

            rayZNear = rayZFar;
            rayZFar = (dPQKj.z * 0.5f + pqk.z) / (dPQKj.w * 0.5f + pqk.w);

            hitPixel = permute ? pqk.yx : pqk.xy;
            hitPixel *= rd.v2_texelSize;

            stride *= ray_intersects_depth_buffer(rayZNear, rayZFar, hitPixel) ? -0.5f : 0.5f;
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
                      vec3 rayOrigin,
                      vec3 rayDirection)
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

    float eyeDirection = clamp(rayDirection.z, eyeFadeStart, eyeFadeEnd);
    alpha *= 1.f - ((eyeDirection - eyeFadeStart) / (eyeFadeEnd - eyeFadeStart));

    // Fade ray hits based on distance from ray origin
    alpha *= 1.f - clamp(distance(rayOrigin, hitPoint) / rd.f_maxRayDistance, 0.f, 1.f);

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
    if(fragMetallic < rd.f_minGlossiness)
    {
        out_SSR = vec4(0.f, 0.f, 0.f, 0.f);
        return;
    }

    vec3 fragNormal = normalize(decompress_normal(fNormMetAO.xy));

    float depth = texture(depthTex, texCoord).r;
    vec3 rayOrigin = reconstruct_position(depth, frag_ray, rd.v4_proj_params);

    vec4 fAlbRough = texture(albedoTex, texCoord);
    float fragRoughness = fAlbRough.a;
    float reflectivity = fragMetallic*(1.f-fragRoughness);
    reflectivity = (reflectivity - rd.f_minGlossiness) / (1.f - rd.f_minGlossiness);
    vec3 rayDirection = normalize(reflect(normalize(rayOrigin), fragNormal));

    vec2 hitPixel;
    vec3 hitPoint;
    float iterationCount;

    // Jitter fraction to hide banding artifacts
    float c = (gl_FragCoord.x + gl_FragCoord.y) * 0.5f;
    float jitter = (0.1f + fract(c)) / 1.1f;

    // TMP dithering
    vec3 wp = vec3(rd.m4_invView * vec4(rayOrigin, 1.0));
    vec3 dither = rd.f_ditherAmount * (hash(wp) * 2.f - 1.f) * fragRoughness;//mix(vec3(0.0), hash(wp), fragRoughness);
    rayDirection = normalize(rayDirection + dither);

    bool intersect = ray_march(rayOrigin, rayDirection, jitter, hitPixel, hitPoint, iterationCount/*, texCoord.x > 0.5*/);

    // Temporal reprojection
    vec4 reproj_hitpx = rd.m4_reproj*vec4(hitPixel, 1.f, 1.f);
    hitPixel = clamp(reproj_hitpx.xy / reproj_hitpx.w, 0.f, 1.f);

    float alpha = ssr_attenuation(intersect, iterationCount, reflectivity, hitPixel, hitPoint, rayOrigin, rayDirection);

    out_SSR = (intersect ? vec4(texture(lastFrameTex, hitPixel).rgb, alpha) : vec4(0.f));
    //out_SSR = vec4(hitPixel.xy, 0.f, alpha);
}
