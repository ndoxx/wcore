#version 400 core

#include "normal_compression.glsl"
#include "position.glsl"
#include "math_utils.glsl"

struct render_data
{
    vec2 v2_texelSize;
    vec2 v2_noiseScale;
    vec3 v3_lightDir;

    float f_radius;
    float f_intensity;
    float f_scale;
    float f_bias;
    float f_vbias;

    float f_inv_far;

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

const float f_occlusion_threshold = 0.5f;

float ambiant_occlusion(vec2 texCoord, vec2 offset, vec3 frag_pos, vec3 frag_normal)
{
    vec3 diff = reconstruct_position(depthTex, texCoord + offset, frag_ray, rd.v4_proj_params) - frag_pos;
    //diff = mix(diff, -frag_normal, rd.f_vbias);
    diff -= rd.f_vbias * frag_normal;
    vec3 v = normalize(diff);
    float d2 = dot(diff,diff)*rd.f_scale;
    return max(0.0f, dot(frag_normal,v)-rd.f_bias)*(1.0f/(1.0f+d2));
}

vec4 directional_occlusion(vec2 texCoord, vec2 offset, vec3 frag_pos, vec3 frag_normal)
{
    vec3 diff = reconstruct_position(depthTex, texCoord + offset, frag_ray, rd.v4_proj_params) - frag_pos;
    //diff = mix(diff, -frag_normal, rd.f_vbias);
    diff -= rd.f_vbias * frag_normal;
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

vec2 poissonDisk[16] = vec2[](
    vec2( -0.94201624, -0.39906216 ),
    vec2( 0.94558609, -0.76890725 ),
    vec2( -0.094184101, -0.92938870 ),
    vec2( 0.34495938, 0.29387760 ),
    vec2( -0.91588581, 0.45771432 ),
    vec2( -0.81544232, -0.87912464 ),
    vec2( -0.38277543, 0.27676845 ),
    vec2( 0.97484398, 0.75648379 ),
    vec2( 0.44323325, -0.97511554 ),
    vec2( 0.53742981, -0.47373420 ),
    vec2( -0.26496911, -0.41893023 ),
    vec2( 0.79197514, 0.19090188 ),
    vec2( -0.24188840, 0.99706507 ),
    vec2( -0.81409955, 0.91437590 ),
    vec2( 0.19984126, 0.78641367 ),
    vec2( 0.14383161, -0.14100790 )
);

vec2 random_sample(vec3 seed, int i)
{
    vec4 seed4 = vec4(seed,i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    float rnd = fract(sin(dot_product) * 43758.5453);
    int index = int(16.0f*rnd)%16;
    return poissonDisk[index];
}

void main()
{
    vec2 texCoord = gl_FragCoord.xy * rd.v2_texelSize;

    // View space
    vec3 fragPos = reconstruct_position(depthTex, texCoord, frag_ray, rd.v4_proj_params);
    vec3 fragNormal = decompress_normal(texture(normalTex, texCoord).xy);

    // Tangent space
    //vec2 randomVec = vec2(rand(texCoord.x), rand(texCoord.y));
    vec2 randomVec = normalize(texture(noiseTex, texCoord*rd.v2_noiseScale).xy);

    /*vec3 tangent = normalize(randomVec - fragNormal * dot(randomVec, fragNormal));
    vec3 bitangent = cross(fragNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, fragNormal);*/

    float occlusion = 0.0f;
    //vec4 occlusion = vec4(0);
    float rad = max(rd.v2_texelSize.x, rd.f_radius/max(0.1f,-fragPos.z));

    int iterations = int(mix(4.0,1.0,fragPos.z*rd.f_inv_far));
    for(int jj=0; jj<iterations; ++jj)
    {
        //vec2 coord1 = reflect(random_sample(texCoord.xyy, jj),randomVec)*rad;
        vec2 coord1 = reflect(SAMPLES[jj],randomVec)*rad;
        vec2 coord2 = PHI * vec2(coord1.x - coord1.y, coord1.x + coord1.y);

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
