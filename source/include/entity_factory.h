#ifndef ENTITY_FACTORY_H
#define ENTITY_FACTORY_H

#include <map>
#include <memory>
#include <functional>

#include "wtypes.h"
#include "xml_parser.h"
#include "wentity.h"

namespace wcore
{

class EntityFactory
{
public:
    // Factory method type for component creation inside an entity
    typedef std::function<bool(WEntity& target, rapidxml::xml_node<>* cmp_node)> ComponentCreatorFunc;

    EntityFactory(const char* entityfile);

    // Associate a component name to a component factory method
    void register_component_factory(hash_t name, ComponentCreatorFunc func);
    // Create an entity from blueprint name
    std::shared_ptr<WEntity> make_entity_blueprint(hash_t name);

private:
    // Parse XML file for entity blueprints
    void parse_entity_file(const char* xmlfile);
    // Parse entity blueprint at given node
    void parse_blueprints(rapidxml::xml_node<>* blueprints_node);

private:
    XMLParser xml_parser_;
    std::map<hash_t, rapidxml::xml_node<>*> blueprints_;
    std::map<hash_t, ComponentCreatorFunc>  component_factories_;
};

} // namespace wcore

#endif // ENTITY_FACTORY_H
