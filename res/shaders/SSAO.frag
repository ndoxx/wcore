#version 400 core

#include "normal_compression.glsl"
#include "position.glsl"
#include "math_utils.glsl"

struct render_data
{
    vec2 v2_screenSize;
    vec2 v2_noiseScale;
    vec3 v3_lightDir;

    float f_radius;
    float f_intensity;
    float f_scale;
    float f_bias;
    float f_vbias;

    bool b_invert_normals;
    // Position reconstruction
    vec4 v4_proj_params;
};

in vec2 frag_ray;

layout(location = 0) out vec4 out_occlusion;

uniform render_data rd;
uniform sampler2D normalTex;
uniform sampler2D noiseTex;
uniform sampler2D depthTex;
uniform sampler2D albedoTex;
//uniform sampler1D randomFieldTex;
//uniform vec3 v3_samples[64];
//const int KERNEL_SIZE = 64;

#define PHI 0.707106f
const vec2 SAMPLES[4] = vec2[](vec2(1,0),vec2(-1,0),
                               vec2(0,1),vec2(0,-1));
const float FAR = 100.0;
const float invFAR = 0.01;
const float f_occlusion_threshold = 0.5;

vec3 get_position(vec2 uv)
{
    float depth  = texture(depthTex, uv).r;
    return reconstruct_position(depth, frag_ray, rd.v4_proj_params);
}

float ambiant_occlusion(vec2 texCoord, vec2 offset, vec3 frag_pos, vec3 frag_normal)
{
    vec3 diff = get_position(texCoord + offset) - frag_pos;
    diff = mix(diff, -frag_normal, rd.f_vbias);
    vec3 v = normalize(diff);
    float d2 = dot(diff,diff)*rd.f_scale;
    //float d = length(diff)*rd.f_scale;
    return max(0.0f, dot(frag_normal,v)-rd.f_bias)*(1.0f/(1.0f+d2));
}

vec4 directional_occlusion(vec2 texCoord, vec2 offset, vec3 frag_pos, vec3 frag_normal)
{
    vec3 diff = get_position(texCoord + offset) - frag_pos;
    diff = mix(diff, -frag_normal, rd.f_vbias);
    vec3 v = normalize(diff);
    float d2 = dot(diff,diff)*rd.f_scale;

    float attenuation = 1.0f/(1.0f+d2);
    float occlusion_amount = max(0.0f, dot(frag_normal,v)-rd.f_bias)*attenuation;

    // GI approx
    vec3 sample_normal = decompress_normal(texture(normalTex, texCoord + offset).xy);
    float bleeding = max(0.0f, dot(sample_normal, -frag_normal))*attenuation;
    vec3 first_bounce = (0.2+0.8*bleeding) * texture(albedoTex, texCoord + offset).rgb;
    return vec4(first_bounce, occlusion_amount);
}

void main()
{
    vec2 texCoord = gl_FragCoord.xy / rd.v2_screenSize;

    // View space
    vec3 fragPos = get_position(texCoord);
    vec3 fragNormal = decompress_normal(texture(normalTex, texCoord).xy);

    // Tangent space
    //vec2 randomVec = vec2(rand(texCoord.x), rand(texCoord.y));
    vec2 randomVec = normalize(texture(noiseTex, texCoord*rd.v2_noiseScale).xy);

    float occlusion = 0.0f;
    //vec4 occlusion = vec4(0);
    float rad = rd.f_radius/fragPos.z;

    int iterations = int(mix(4.0,1.0,fragPos.z*invFAR));
    for (int jj=0; jj<iterations; ++jj)
    {
        vec2 coord1 = reflect(SAMPLES[jj],randomVec)*rad;
        vec2 coord2 = vec2(coord1.x*0.707f - coord1.y*0.707f, coord1.x*0.707f + coord1.y*0.707f);

        occlusion += ambiant_occlusion(texCoord, coord1*0.25f, fragPos, fragNormal);
        occlusion += ambiant_occlusion(texCoord, coord2*0.5f,  fragPos, fragNormal);
        occlusion += ambiant_occlusion(texCoord, coord1*0.75f, fragPos, fragNormal);
        occlusion += ambiant_occlusion(texCoord, coord2*1.0f,  fragPos, fragNormal);
    }
    occlusion /= float(iterations*4);
    occlusion = max(1.0-(rd.f_intensity*occlusion), 0.0);
    //occlusion.a = max(1.0-(rd.f_intensity*occlusion.a), 0.0);

    out_occlusion.r = occlusion;
    //out_occlusion = occlusion;
}
