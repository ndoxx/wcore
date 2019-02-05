#include "value_map.h"
#include "logger.h"

namespace wcore
{

void ValueMap::parse_xml_file(const fs::path& path)
{
    xml_parser_.load_file_xml(path);
    retrieve_configuration(xml_parser_.get_root(), "root");
}

// Recursive parser
void ValueMap::retrieve_configuration(rapidxml::xml_node<>* node,
                                      const std::string& name_chain)
{
    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node=node->first_node();
        cur_node;
        cur_node=cur_node->next_sibling())
    {
        // Look for children node if any
        rapidxml::xml_node<>* child_node = cur_node->first_node();
        if(child_node)
        {
            // Get current node name and append to chain
            const char* node_name = cur_node->name();
            std::string chain(name_chain + "." + node_name);
            // Get configuration for next level
            retrieve_configuration(cur_node, chain);
        }
        else
        {
            // If no child, then try to extract property
            hash_t name_hash = parse_xml_property(cur_node, name_chain);
            if(name_hash)
            {
                dom_locations_.insert(std::make_pair(name_hash, cur_node));
            }
            else // Node is invalid
            {
                DLOGW("[ValueMap] Ignoring ill-formed property node.", "core", Severity::WARN);
                DLOGI("At: " + name_chain + ": " + cur_node->name(), "core", Severity::WARN);
            }
        }
    }
}

hash_t ValueMap::parse_xml_property(rapidxml::xml_node<>* node,
                                  const std::string& name_chain)
{
    std::string str_var_name;
    if(!xml::parse_attribute(node, "name", str_var_name))
        return 0;

    std::string str_full_name(name_chain + "." + str_var_name);
    hash_t full_name_hash = H_(str_full_name.c_str());

    // Get hash from node name
    hash_t nameHash = H_(node->name());

    switch(nameHash)
    {
        case "uint"_h:
        {
            uint32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case "int"_h:
        {
            int32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case "float"_h:
        {
            float value = 0.0f;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case "vec2"_h:
        {
            math::vec2 value(0.0f);
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case "vec3"_h:
        {
            math::vec3 value(0.0f);
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case "string"_h:
        {
            std::string value;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value.c_str());
            break;
        }
        case "path"_h:
        {
            std::string value;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            fs::path dir(root_path_ / fs::path(value.c_str()));
            if(fs::exists(dir))
                this->set(full_name_hash, std::cref(dir));
            else
            {
                DLOGE("[ValueMap] Directory does not exist: ", "core", Severity::CRIT);
                DLOGI(dir.string(), "core", Severity::CRIT);
                DLOGI("Skipping.", "core", Severity::CRIT);
                return 0;
            }
            break;
        }
        case "bool"_h:
        {
            bool value = false;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
    }

    return full_name_hash;
}

} // namespace wcore
