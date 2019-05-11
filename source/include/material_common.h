#ifndef MATERIAL_COMMON_H
#define MATERIAL_COMMON_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>

#include "wtypes.h"
#include "math3d.h"

namespace wcore
{

enum TextureFilter: uint8_t
{
    MAG_NEAREST = 0,
    MAG_LINEAR  = 1,

    MIN_NEAREST = 2,
    MIN_LINEAR  = 4,
    MIN_NEAREST_MIPMAP_NEAREST = 8,
    MIN_LINEAR_MIPMAP_NEAREST  = 16,
    MIN_NEAREST_MIPMAP_LINEAR  = 32,
    MIN_LINEAR_MIPMAP_LINEAR   = 64
};

enum class TextureWrap: uint8_t
{
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE
};

enum class TextureUnit: uint16_t
{
    ALBEDO    = 1,
    AO        = 2,
    DEPTH     = 4,
    METALLIC  = 8,
    NORMAL    = 16,
    ROUGHNESS = 32,
    BLOCK0    = 64,
    BLOCK1    = 128,
    BLOCK2    = 256
};

struct TextureParameters
{
    TextureFilter filter;
    uint32_t internal_format;
    uint32_t format;
    TextureWrap wrap;
    bool lazy_mipmap;

    TextureParameters();
};

struct TextureDescriptor
{
    typedef std::map<TextureUnit, std::string> TexMap;

    // Image texture file names by map type
    TexMap locations;
    // Flags for each unit
    uint16_t unit_flags;
    // Sampler group number
    uint8_t sampler_group;
    // OpenGL texture parameters
    TextureParameters parameters;
    // Unique id
    hash_t resource_id;

    // Wat format
    bool is_wat;
    bool owns_data;
    std::string wat_location;
    uint32_t width;
    uint32_t height;
    unsigned char* block0_data; // Pointers to pixel data for block 0
    unsigned char* block1_data; // Pointers to pixel data for block 1
    unsigned char* block2_data; // Pointers to pixel data for block 2

    TextureDescriptor();
    ~TextureDescriptor();

    inline bool has_unit(TextureUnit unit) const { return (unit_flags&(uint16_t)unit); }
    inline void add_unit(TextureUnit unit)       { unit_flags |= (uint16_t)unit; }
    void release_data();
};

struct MaterialDescriptor
{
    TextureDescriptor texture_descriptor;

    // Uniform alternatives
    math::vec3 albedo;
    float      transparency;
    float      metallic;
    float      roughness;
    bool       has_transparency;
    bool       is_textured;

    // Shading information
    float parallax_height_scale;

    // Override
    bool enable_normal_mapping;
    bool enable_parallax_mapping;

    MaterialDescriptor();

#ifdef __DEBUG__
    friend std::ostream& operator<< (std::ostream& stream, const MaterialDescriptor& desc);
#endif
};

struct CubemapDescriptor
{
    std::vector<std::string> locations;
    hash_t resource_id;
};

}

#endif // MATERIAL_COMMON_H
