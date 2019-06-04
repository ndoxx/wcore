#ifndef MATERIAL_COMMON_H
#define MATERIAL_COMMON_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>

#include "texture.h"
#include "math3d.h"

namespace wcore
{

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
