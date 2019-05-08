#ifndef MATERIAL_FACTORY_H
#define MATERIAL_FACTORY_H

#include <map>
#include <variant>
#include <random>
#include <istream>
#include <memory>
#include "xml_parser.h"
#include "material_common.h"

namespace wcore
{

class Material;
class Texture;
class Cubemap;
class WatLoader;
class MaterialFactory
{
private:
    typedef std::map<hash_t, MaterialDescriptor> MaterialMap;
    typedef std::map<hash_t, CubemapDescriptor> CubemapsMap;

    XMLParser xml_parser_;
    MaterialMap material_descriptors_;
    CubemapsMap cubemap_descriptors_;
    std::map<hash_t, Material*> cache_;
    static std::map<TextureUnit, const char*> TEX_SAMPLERS_NODES;

    std::map<hash_t, std::shared_ptr<Texture>> texture_cache_;

public:
    MaterialFactory(const char* xml_file);
    MaterialFactory();
    ~MaterialFactory();

    typedef std::mt19937* OptRngT;

    void retrieve_material_descriptions(rapidxml::xml_node<>* materials_node);
    void retrieve_cubemap_descriptions(rapidxml::xml_node<>* cubemaps_node);
    Material* make_material(hash_t asset_name, uint8_t sampler_group=1);
    Material* make_material(MaterialDescriptor& descriptor);
    Material* make_material(rapidxml::xml_node<>* material_node,
                            uint8_t sampler_group=1,
                            OptRngT opt_rng=nullptr);
    Texture* make_texture(std::istream& stream);
    Cubemap* make_cubemap(hash_t cubemap_name);

    inline const MaterialDescriptor& get_material_descriptor(hash_t asset_name) { return material_descriptors_.at(asset_name); }
    inline const CubemapDescriptor& get_cubemap_descriptor(hash_t asset_name)   { return cubemap_descriptors_.at(asset_name); }

    void parse_material_descriptor(rapidxml::xml_node<>* node,
                                   MaterialDescriptor& descriptor,
                                   OptRngT opt_rng=nullptr);
    void parse_cubemap_descriptor(rapidxml::xml_node<>* node,
                                  CubemapDescriptor& descriptor);

    // Remove cached textures that are not shared anymore
    void cache_cleanup();

private:
    WatLoader* wat_loader_;
};

}

#endif // MATERIAL_FACTORY_H
