#ifndef MATERIAL_FACTORY_H
#define MATERIAL_FACTORY_H

#include <ostream>
#include <map>
#include <vector>
#include <string>
#include "xml_parser.h"
#include "utils.h"
#include "math3d.h"

class Material;

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
    TexMap    locations;
    // Flags for each unit
    uint16_t units = 0;
    // OpenGL texture parameters
    TextureParameters parameters;
    // Unique id
    hash_t resource_id;

    inline bool has_unit(TextureUnit unit) const { return (units&(uint16_t)unit); }
    inline void add_unit(TextureUnit unit)       { units |= (uint16_t)unit; }
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

    // Shading information
    float parallax_height_scale = 0.1f;

    // Override
    bool enable_normal_mapping = true;
    bool enable_parallax_mapping = true;

#ifdef __DEBUG__
    friend std::ostream& operator<< (std::ostream& stream, const MaterialDescriptor& desc);
#endif
};

class MaterialFactory
{
private:
    typedef std::map<hash_t, MaterialDescriptor> AssetMap;

    XMLParser xml_parser_;
    AssetMap  material_descriptors_;
    static std::map<hashstr_t, TextureUnit> NAME_TO_TEXTURE_SAMPLER;
    static std::map<TextureUnit, const char*> TEX_SAMPLERS_NODES;

public:
    MaterialFactory(const char* filename);
    ~MaterialFactory();

    void retrieve_asset_descriptions(rapidxml::xml_node<>* root);
    Material* make_material(hash_t asset_name);

    inline const MaterialDescriptor& get_descriptor(hash_t asset_name)     { return material_descriptors_.at(asset_name); }
    static inline TextureUnit texture_sampler_from_name(hashstr_t name) { return NAME_TO_TEXTURE_SAMPLER.at(name); }

private:
    void parse_material_descriptor(rapidxml::xml_node<>* node, MaterialDescriptor& descriptor);
};

#endif // MATERIAL_FACTORY_H
