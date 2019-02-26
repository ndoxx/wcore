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
    ROUGHNESS = 32
};

struct TextureParameters
{
public:
    uint32_t filter;
    uint32_t internal_format;
    uint32_t format;
    bool clamp;
    bool lazy_mipmap;

    TextureParameters();
};

struct TextureDescriptor
{
public:
    typedef std::map<TextureUnit, std::string> TexMap;

    // Image texture file names by map type
    TexMap locations;
    // Flags for each unit
    uint16_t units = 0;
    // Sampler group number
    uint8_t sampler_group = 1;
    uint8_t n_units = 0;
    // OpenGL texture parameters
    TextureParameters parameters;
    // Unique id
    hash_t resource_id;

    inline bool has_unit(TextureUnit unit) const { return (units&(uint16_t)unit); }
    inline void add_unit(TextureUnit unit)       { units |= (uint16_t)unit; ++n_units; }
};

struct MaterialDescriptor
{
public:
    TextureDescriptor texture_descriptor;

    // Uniform alternatives
    math::vec3 albedo = math::vec3(1.0f,0.0f,0.0f);
    float      transparency = 1.0f;
    float      metallic = 0.0f;
    float      roughness = 0.1f;
    bool       has_transparency = false;
    bool       is_textured = false;

    // Shading information
    float parallax_height_scale = 0.1f;

    // Override
    bool enable_normal_mapping = true;
    bool enable_parallax_mapping = true;

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
