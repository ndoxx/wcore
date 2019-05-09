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
    uint32_t filter;
    uint32_t internal_format;
    uint32_t format;
    bool clamp;
    bool lazy_mipmap;

    TextureParameters();
};

struct TextureDescriptor
{
    typedef std::map<TextureUnit, std::string> TexMap;

    // Image texture file names by map type
    TexMap locations;
    // Flags for each unit
    uint16_t units;
    // Sampler group number
    uint8_t sampler_group;
    // OpenGL texture parameters
    TextureParameters parameters;
    // Unique id
    hash_t resource_id;

    TextureDescriptor();

    inline bool has_unit(TextureUnit unit) const { return (units&(uint16_t)unit); }
    inline void add_unit(TextureUnit unit)       { units |= (uint16_t)unit; }
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

    // Wat format
    bool is_wat;
    std::string wat_location;

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
