#include "material_factory.h"


MaterialFactory::MaterialFactory(const char* filename)
{
    xml_parser_.load_file_xml(filename);
    retrieve_asset_descriptions(xml_parser_.get_root());
}

MaterialFactory::~MaterialFactory()
{

}

void MaterialFactory::retrieve_asset_descriptions(rapidxml::xml_node<>* root)
{

}
