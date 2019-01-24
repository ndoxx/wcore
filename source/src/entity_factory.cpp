#include <filesystem>

#include "entity_factory.h"
#include "io_utils.h"
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
    fs::path file_path(io::get_file(H_("root.folders.level"), xmlfile));
    xml_parser_.load_file_xml(file_path);
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


} // namespace wcore
