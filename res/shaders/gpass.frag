#version 400 core

#include "normal_compression.glsl"
#include "parallax.glsl"

struct material
{
    sampler2D albedoTex;
    sampler2D normalTex;
    sampler2D depthTex;
    sampler2D overlayTex;

    sampler2D AOTex;
    sampler2D metallicTex;
    sampler2D roughnessTex;

    float f_roughness;
    float f_parallax_height_scale;
    vec3 v3_tint;
    bool b_is_textured;
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

#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
layout(location = 0) out vec4 out_normal;
layout(location = 1) out vec4 out_albedo;
#else
layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_albedo;
#endif

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
        vec3 viewDir = frag_tangent_viewDir;//normalize(frag_tangent_fragPos-frag_tangent_viewPos);
        viewDir.y = -viewDir.y; // WTF? else parallax mapping would give wrong perspective along world z axis
        if(mt.b_use_parallax_map)
            texCoords = parallax_map(frag_texCoord, viewDir, -mt.f_parallax_height_scale, mt.depthTex);
            // WTF minus mt.f_parallax_height_scale? else inverted depth

        // Normal vector from normal map
        normal = texture(mt.normalTex, texCoords).rgb;
        normal = normalize(normal*2.0 - 1.0);
        normal = normalize(frag_TBN*normal);
    }
    else
    {
        // Normal vector interpolated at fragment position
        normal = normalize(frag_normal);
    }

    vec2 normal_cmp = compress_normal(normal);
    // DEBUG wireframe color
    float wireframe = edge_factor() * rd.f_wireframe_mix;

    if(mt.b_is_textured)
    {
#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
        vec3 albedo  = mix(texture(mt.albedoTex, texCoords).rgb, vec3(0.5f), wireframe);
        out_albedo   = vec4(albedo, texture(mt.roughnessTex, texCoords).r);
#else
        out_albedo   = vec4(texture(mt.albedoTex, texCoords).rgb, wireframe);
        out_position = vec4(frag_pos, texture(mt.roughnessTex, texCoords).r);
#endif
        out_normal   = vec4(normal_cmp,
                            texture(mt.metallicTex, texCoords).r,
                            texture(mt.AOTex, texCoords).r);
    }
    else
    {
#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
        vec3 albedo  = mix(mt.v3_tint, vec3(0.5f), wireframe);
        out_albedo   = vec4(albedo, mt.f_roughness);
#else
        out_albedo   = vec4(mt.v3_tint, wireframe);
        out_position = vec4(frag_pos, mt.f_roughness);
#endif
        out_normal   = vec4(normal_cmp, 0.0, 1.0);
    }

/*
    // Display LoD as green tint
    float lod = textureQueryLod(mt.albedoTex, texCoords).y;
    if(lod>=PARALLAX_LOD_THRESHOLD)
        out_albedo.g = 1.0;
*/

    // DEBUG Overlay mask
    if(mt.b_has_overlay)
        out_albedo.a += step(0.5, texture(mt.overlayTex, texCoords).g);
}

// Function to compute fragment distance to edges
float edge_factor()
{
    vec3 d = fwidth(frag_barycentric);
    vec3 a3 = smoothstep(vec3(0.0), d*1.0, frag_barycentric);
    return 1.0-min(min(a3.x, a3.y), a3.z);
}
