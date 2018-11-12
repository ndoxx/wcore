#ifndef MATERIAL_FACTORY_H
#define MATERIAL_FACTORY_H

#include "xml_parser.h"

class MaterialFactory
{
private:
    XMLParser xml_parser_;

public:
    MaterialFactory(const char* filename);
    ~MaterialFactory();

    void retrieve_asset_descriptions(rapidxml::xml_node<>* root);

};

#endif // MATERIAL_FACTORY_H
