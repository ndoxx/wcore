#ifndef MATERIAL_FACTORY_H
#define MATERIAL_FACTORY_H

#include <ostream>
#include <map>
#include <vector>
#include <string>
#include "xml_parser.h"
#include "utils.h"
#include "math3d.h"

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
    typedef std::map<hashstr_t, std::string> TexMap;
    typedef std::map<hashstr_t, bool> HasTexMap;
    //typedef std::map<hashstr_t, TextureParameters> ParamMap;

    // Image texture file names by map type
    TexMap    locations;
    // Flags for each unit
    HasTexMap has_unit;
    // OpenGL texture parameters
    //ParamMap  parameters;
    TextureParameters parameters;
    // Unique id
    hash_t resource_id;
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

public:
    MaterialFactory(const char* filename);
    ~MaterialFactory();

    void retrieve_asset_descriptions(rapidxml::xml_node<>* root);

    inline const MaterialDescriptor& get_descriptor(hash_t asset_name) { return material_descriptors_.at(asset_name); }

private:
    void parse_material_descriptor(rapidxml::xml_node<>* node, MaterialDescriptor& descriptor);
};

#endif // MATERIAL_FACTORY_H
