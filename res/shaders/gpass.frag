#version 400 core

#include "normal_compression.glsl"
#include "parallax.glsl"

struct sampler_group
{
    sampler2D albedoTex;
    sampler2D AOTex;
    sampler2D metallicTex;
    sampler2D roughnessTex;
    sampler2D normalTex;
    sampler2D depthTex;
};

struct material
{
    sampler_group sg1;
#ifdef VARIANT_SPLAT
    sampler_group sg2;
#endif

    float f_parallax_height_scale;

    vec3  v3_albedo;
    float f_roughness;
    float f_metallic;

    bool b_has_albedo;
    bool b_has_ao;
    bool b_has_metallic;
    bool b_has_roughness;

    bool b_use_normal_map;
    bool b_use_parallax_map;
    bool b_has_overlay;
};

struct render_data
{
    float f_wireframe_mix;  // Wireframe blend factor in[0,1]
    vec3 v3_viewPos;
};

in vec3 frag_pos;
in vec3 frag_normal;
in vec3 frag_barycentric;
in vec3 frag_tangent_viewDir;
in vec2 frag_texCoord;
in mat3 frag_TBN;

layout(location = 0) out vec4 out_normal;
layout(location = 1) out vec4 out_albedo;

// UNIFORMS
uniform material mt;
uniform render_data rd;

float edge_factor();

// Write to g-buffer
void main()
{
    // Do we use normal+parallax mapping?
    vec2 texCoords = frag_texCoord;
    vec3 normal;
    if(mt.b_use_normal_map)
    {
        vec3 viewDir = -frag_tangent_viewDir;//normalize(frag_tangent_fragPos-frag_tangent_viewPos);
        //viewDir.y=-viewDir.y;
        if(mt.b_use_parallax_map)
            texCoords = parallax_map(frag_texCoord, viewDir, mt.f_parallax_height_scale, mt.sg1.depthTex);

        // Normal vector from normal map
        normal = texture(mt.sg1.normalTex, texCoords).rgb;
        normal = normalize(normal*2.0 - 1.0);
        normal = normalize(frag_TBN*normal);
    }
    else
    {
        // Normal vector interpolated at fragment position
        normal = normalize(frag_normal);
    }

    vec2 normal_cmp = compress_normal(normal);

    vec3  albedo    = mt.b_has_albedo?    texture(mt.sg1.albedoTex, texCoords).rgb:  mt.v3_albedo;
    float roughness = mt.b_has_roughness? texture(mt.sg1.roughnessTex, texCoords).r: mt.f_roughness;
    float metallic  = mt.b_has_metallic?  texture(mt.sg1.metallicTex, texCoords).r:  mt.f_metallic;
    float ao        = mt.b_has_ao?        texture(mt.sg1.AOTex, texCoords).r:        1.0f;

    // DEBUG wireframe color
    float wireframe = edge_factor() * rd.f_wireframe_mix;

    albedo = mix(albedo, vec3(0.5f), wireframe);
    out_albedo = vec4(albedo, roughness);
    out_normal = vec4(normal_cmp, metallic, ao);

/*
    // Display LoD as green tint
    float lod = textureQueryLod(mt.albedoTex, texCoords).y;
    if(lod>=PARALLAX_LOD_THRESHOLD)
        out_albedo.g = 1.0;
*/
}

// Function to compute fragment distance to edges
float edge_factor()
{
    vec3 d = fwidth(frag_barycentric);
    vec3 a3 = smoothstep(vec3(0.0), d*1.0, frag_barycentric);
    return 1.0-min(min(a3.x, a3.y), a3.z);
}
