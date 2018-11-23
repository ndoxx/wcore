#include "config.h"
#include "xml_utils.hpp"
#include "logger.h"

namespace wcore
{

using namespace rapidxml;

void Config::load_file_xml(const char* xml_file)
{
    DLOGN("[Config] Parsing xml configuration file:", "default", Severity::LOW);
    xml_parser_.load_file_xml(xml_file);

    retrieve_configuration(xml_parser_.get_root(), "root");
}

// Recursive parser
void Config::retrieve_configuration(rapidxml::xml_node<>* node,
                                    const std::string& name_chain)
{
    // For each siblings at this recursion level
    for(xml_node<>* cur_node=node->first_node();
        cur_node;
        cur_node=cur_node->next_sibling())
    {
        // Look for children node if any
        xml_node<>* child_node = cur_node->first_node();
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
            #ifndef __PRESERVE_STRS__
            if(name_hash)
            {
                dom_locations_.insert(std::make_pair(name_hash, cur_node));
            }
            #else
            if(name_hash.size())
            {
                dom_locations_.insert(std::make_pair(name_hash, cur_node));
            }
            #endif
            else // Node is invalid
            {
                DLOGW("[Config] Ignoring ill-formed property node.", "default", Severity::WARN);
                DLOGI("At: " + name_chain + ": " + cur_node->name(), "default", Severity::WARN);
            }
        }
    }
}

static const hashstr_t H_UINT   = HS_("uint");
static const hashstr_t H_INT    = HS_("int");
static const hashstr_t H_FLOAT  = HS_("float");
static const hashstr_t H_STRING = HS_("string");
static const hashstr_t H_BOOL   = HS_("bool");


hash_t Config::parse_xml_property(rapidxml::xml_node<>* node,
                                  const std::string& name_chain)
{
    std::string str_var_name;
    if(!xml::parse_attribute(node, "name", str_var_name))
        return 0;

    std::string str_full_name(name_chain + "." + str_var_name);
    hash_t full_name_hash = H_(str_full_name.c_str());

    // Get hash from node name
    hashstr_t nameHash = HS_(node->name());

    switch(nameHash)
    {
        case H_UINT:
        {
            uint32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case H_INT:
        {
            int32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case H_FLOAT:
        {
            float value = 0.0f;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case H_STRING:
        {
            std::string value;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value.c_str());
            break;
        }
        case H_BOOL:
        {
            bool value = false;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
    }

    return full_name_hash;
}

void Config::debug_display_content()
{
    std::cout << "--uints--" << std::endl;
    for(auto&& [key, value]: uints_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--ints--" << std::endl;
    for(auto&& [key, value]: ints_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--floats--" << std::endl;
    for(auto&& [key, value]: floats_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--strings--" << std::endl;
    for(auto&& [key, value]: strings_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--bools--" << std::endl;
    for(auto&& [key, value]: bools_)
        std::cout << "    " << key << " -> " << value << std::endl;
}

}
