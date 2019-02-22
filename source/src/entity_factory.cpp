#include <filesystem>

#include "entity_factory.h"
#include "file_system.h"
#include "error.h"
#include "xml_utils.hpp"
#include "logger.h"

namespace wcore
{

EntityFactory::EntityFactory(const char* entityfile)
{
    parse_entity_file(entityfile);
    rapidxml::xml_node<>* blueprints_node = xml_parser_.get_root()->first_node("Blueprints");
    parse_blueprints(blueprints_node);
}

void EntityFactory::parse_entity_file(const char* xmlfile)
{
    auto pstream = FILESYSTEM.get_file_as_stream(xmlfile, "root.folders.level"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGE("[EntityFactory] Unable to open file:", "entity", Severity::CRIT);
        DLOGI(xmlfile, "entity", Severity::CRIT);
        fatal();
    }
    xml_parser_.load_file_xml(*pstream);
}

void EntityFactory::parse_blueprints(rapidxml::xml_node<>* blueprints_node)
{
    for (rapidxml::xml_node<>* ent_node=blueprints_node->first_node("Entity");
         ent_node;
         ent_node=ent_node->next_sibling("Entity"))
    {
        std::string ent_name;
        if(xml::parse_attribute(ent_node, "name", ent_name))
        {
#ifdef __DEBUG__
            if(blueprints_.find(H_(ent_name.c_str())) != blueprints_.end())
            {
                DLOGW("[EntityFactory] Entity redefinition or collision: ", "entity", Severity::WARN);
                DLOGI(ent_name, "entity", Severity::WARN);
            }
#endif
            blueprints_[H_(ent_name.c_str())] = ent_node;
#ifdef __DEBUG__
            HRESOLVE.add_intern_string(ent_name);
#endif
        }
    }
}

void EntityFactory::register_component_factory(hash_t name, ComponentCreatorFunc func)
{
#ifdef __DEBUG__
    if(component_factories_.find(name) != component_factories_.end())
    {
        DLOGW("[EntityFactory] Component creator function redefinition or collision: ", "entity", Severity::WARN);
        DLOGI(std::to_string(name) + " -> " + HRESOLVE(name), "entity", Severity::WARN);
    }
#endif
    component_factories_[name] = func;
}

std::shared_ptr<WEntity> EntityFactory::make_entity_blueprint(hash_t name)
{
    auto it = blueprints_.find(name);
    if(it == blueprints_.end())
    {
        DLOGE("[EntityFactory] Entity blueprint name not found: ", "entity", Severity::CRIT);
        DLOGI(std::to_string(name) + " -> " + HRESOLVE(name), "entity", Severity::CRIT);
        return nullptr;
    }

    std::shared_ptr<WEntity> entity(new WEntity());

    // Create each component using registered factories
    rapidxml::xml_node<>* components_node = it->second->first_node("Components");
    for (rapidxml::xml_node<>* cmp_node=components_node->first_node();
         cmp_node;
         cmp_node=cmp_node->next_sibling())
    {
        // Component name is node name
        hash_t cmp_name = H_(cmp_node->name());
        auto fac_it = component_factories_.find(cmp_name);
        if(fac_it == component_factories_.end())
        {
            DLOGE("[EntityFactory] Component creator function not found: ", "entity", Severity::WARN);
            DLOGI(cmp_node->name(), "entity", Severity::WARN);
            DLOGI("Skipping.", "entity", Severity::WARN);
            continue;
        }
        if(!(fac_it->second)(*entity, cmp_node))
        {
            DLOGW("[EntityFactory] Component creation failed: ", "entity", Severity::WARN);
            DLOGI(cmp_node->name(), "entity", Severity::WARN);
            DLOGI("Skipping.", "entity", Severity::WARN);
        }
    }

    return entity;
}


} // namespace wcore
