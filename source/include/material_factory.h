#ifndef MATERIAL_FACTORY_H
#define MATERIAL_FACTORY_H

#include <map>
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
    static std::map<TextureUnit, const char*> TEX_SAMPLERS_NODES;

public:
    MaterialFactory(const char* xml_file);
    ~MaterialFactory();

    void retrieve_asset_descriptions(rapidxml::xml_node<>* root);
    Material* make_material(hash_t asset_name);

    inline const MaterialDescriptor& get_descriptor(hash_t asset_name)  { return material_descriptors_.at(asset_name); }

private:
    void parse_material_descriptor(rapidxml::xml_node<>* node, MaterialDescriptor& descriptor);
};

}

#endif // MATERIAL_FACTORY_H
