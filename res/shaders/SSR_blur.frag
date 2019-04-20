#version 400 core

#include "normal_compression.glsl"
#include "position.glsl"

struct render_data
{
    vec2 v2_texelSize;
    vec4 v4_proj_params;
    vec3 v3_texelOffsetScale;

    float f_depthBias;
    float f_normalBias;
    float f_blurQuality;
};
uniform render_data rd;

uniform sampler2D normalTex;
uniform sampler2D albedoTex;
uniform sampler2D depthTex;
uniform sampler2D mainTex;

const float weights[8] = float[8](0.071303, 0.131514, 0.189879, 0.321392, 0.452906, 0.584419, 0.715932, 0.847445);

layout(location = 0) out vec4 out_color;

float compare_normal_depth(vec3 sourceNormal, float sourceDepth, vec2 uv)
{
    vec3 otherNormal = normalize(decompress_normal(texture(normalTex, uv).xy));
    float otherDepth = depth_view_from_tex(depthTex, uv, rd.v4_proj_params.zw);

    vec3 normalDelta = abs(otherNormal - sourceNormal);
    float depthDelta = abs(otherDepth - sourceDepth);

    return step(normalDelta.x + normalDelta.y + normalDelta.z, rd.f_normalBias)
         * step(depthDelta, rd.f_depthBias);
}

void process_sample(vec2 uv,
                    vec3 sourceNormal,
                    float sourceDepth,
                    float i,
                    float blur_quality, //sampleCount
                    vec2 stepSize,
                    inout vec4 accumulator,
                    inout float denominator)
{
    vec2 offsetUV = stepSize * i + uv;
    float isSame = compare_normal_depth(sourceNormal, sourceDepth, offsetUV);
    float coefficient = weights[int(blur_quality - abs(i))] * isSame;
    accumulator += texture(mainTex, offsetUV) * coefficient;
    denominator += coefficient;
}

void main()
{
    vec2 texCoord = gl_FragCoord.xy * rd.v2_texelSize;

    vec4 fAlbRough = texture(albedoTex, texCoord);
    vec4 fNormMetAO = texture(normalTex, texCoord);

    float sourceDepth = depth_view_from_tex(depthTex, texCoord, rd.v4_proj_params.zw) /*- 0.1f*/;
    float fragRoughness = fAlbRough.a;
    vec3 sourceNormal = normalize(decompress_normal(fNormMetAO.xy));

    vec2 stepSize = rd.v3_texelOffsetScale.xy * fragRoughness;
    vec4 accumulator = texture(mainTex, texCoord) * 0.214607f;
    float denominator = 0.214607f;

    process_sample(texCoord, sourceNormal, sourceDepth,  1,        rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 0.2f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 0.4f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 0.6f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 0.8f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 1.2f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 1.4f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 1.6f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 1.8f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth,  1 * 2.0f, rd.f_blurQuality, stepSize, accumulator, denominator);

    process_sample(texCoord, sourceNormal, sourceDepth, -1,        rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 0.2f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 0.4f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 0.6f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 0.8f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 1.2f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 1.4f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 1.6f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 1.8f, rd.f_blurQuality, stepSize, accumulator, denominator);
    process_sample(texCoord, sourceNormal, sourceDepth, -1 * 2.0f, rd.f_blurQuality, stepSize, accumulator, denominator);

    out_color = accumulator / denominator;
}
