#ifndef MATERIAL_FACTORY_H
#define MATERIAL_FACTORY_H

#include <map>
#include <variant>
#include <random>
#include "xml_parser.h"
#include "material_common.h"

namespace wcore
{

class Material;
class MaterialFactory
{
private:
    typedef std::map<hash_t, MaterialDescriptor> AssetMap;

    XMLParser xml_parser_;
    AssetMap  material_descriptors_;
    std::map<hash_t, Material*> cache_;
    static std::map<TextureUnit, const char*> TEX_SAMPLERS_NODES;

public:
    MaterialFactory(const char* xml_file);
    MaterialFactory();
    ~MaterialFactory();

    //typedef std::variant<std::monostate, std::mt19937> VarRngT;
    typedef std::optional<std::mt19937> VarRngT;

    void retrieve_asset_descriptions(rapidxml::xml_node<>* materials_node);
    Material* make_material(hash_t asset_name);
    Material* make_material(MaterialDescriptor& descriptor);
    Material* make_material(rapidxml::xml_node<>* material_node,
                            VarRngT opt_rng={});

    inline const MaterialDescriptor& get_descriptor(hash_t asset_name)  { return material_descriptors_.at(asset_name); }
    void parse_material_descriptor(rapidxml::xml_node<>* node,
                                   MaterialDescriptor& descriptor,
                                   VarRngT opt_rng={});

private:
};

}

#endif // MATERIAL_FACTORY_H
