#version 400 core

#include "normal_compression.glsl"

struct render_data
{
    vec2 v2_screenSize;
    vec2 v2_noiseScale;
    vec3 v3_lightDir;

    float f_radius;
    float f_intensity;
    float f_scale;
    float f_bias;

    bool b_invert_normals;
};

layout(location = 0) out vec3 out_occlusion;

uniform render_data rd;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D noiseTex;
//uniform sampler1D randomFieldTex;
//uniform vec3 v3_samples[64];
//const int KERNEL_SIZE = 64;

const vec2 SAMPLES[6] = vec2[](vec2(1,0),vec2(-1,0),
                               vec2(0,1),vec2(0,-1),
                               vec2(-0.5,1),vec2(0.5,-1));
const float FAR = 100.0;
const float invFAR = 0.01;
const float f_occlusion_threshold = 0.5;

vec3 get_position(vec2 uv)
{
    return texture(positionTex, uv).xyz;
}

float ambiant_occlusion(vec2 texCoord, vec2 uv, vec3 p, vec3 cnorm)
{
    vec3 diff = get_position(texCoord + uv) - p;
    vec3 v = normalize(diff);
    float d2 = dot(diff,diff)*rd.f_scale;
    return max(0.0, dot(cnorm,v)-rd.f_bias)*(1.0/(1.0+d2));
}

void main()
{
    vec2 texCoord = gl_FragCoord.xy / rd.v2_screenSize;

    // View space
    vec3 fragPos = texture(positionTex, texCoord).xyz;
    vec3 fragNormal = decompress_normal(texture(normalTex, texCoord).xy);

    // Tangent space
    vec2 randomVec = normalize(texture(noiseTex, texCoord*rd.v2_noiseScale).xy);

    // Light direction factor
    float directionFactor = max(dot(rd.v3_lightDir, fragNormal), 0.7);

    float occlusion = 0.0;
    float rad = rd.f_radius/fragPos.z;
    //int iterations = 4;
    int iterations = int(mix(6.0,2.0,fragPos.z*invFAR));
    for (int jj=0; jj<iterations; ++jj)
    {
        vec2 coord1 = reflect(SAMPLES[jj],randomVec)*rad;
        //vec2 coord1 = SAMPLES[jj]*rad;
        vec2 coord2 = vec2(coord1.x*0.707 - coord1.y*0.707, coord1.x*0.707 + coord1.y*0.707);

        occlusion += ambiant_occlusion(texCoord, coord1*0.25, fragPos, fragNormal);
        occlusion += ambiant_occlusion(texCoord, coord2*0.5, fragPos, fragNormal);
        occlusion += ambiant_occlusion(texCoord, coord1*0.75, fragPos, fragNormal);
        occlusion += ambiant_occlusion(texCoord, coord2, fragPos, fragNormal);
    }
    occlusion /= (iterations*4);
    occlusion = max(1.0-(directionFactor*rd.f_intensity*occlusion), 0.0);

    out_occlusion.r = occlusion;
}
