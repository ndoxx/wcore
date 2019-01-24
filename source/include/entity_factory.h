#ifndef ENTITY_FACTORY_H
#define ENTITY_FACTORY_H

#include <map>

#include "wtypes.h"
#include "xml_parser.h"

namespace wcore
{

class EntityFactory
{
public:
    EntityFactory(const char* entityfile);

private:
    void parse_entity_file(const char* xmlfile);
    void parse_blueprints(rapidxml::xml_node<>* blueprints_node);

private:
    XMLParser xml_parser_;
    std::map<hash_t, rapidxml::xml_node<>*> blueprints_;
};

} // namespace wcore

#endif // ENTITY_FACTORY_H
