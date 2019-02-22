#include "value_map.h"
#include "logger.h"

namespace wcore
{

// deprec
void ValueMap::parse_xml_file(const fs::path& path)
{
    xml_parser_.load_file_xml(path);
    retrieve_configuration(xml_parser_.get_root(), "root");
}

void ValueMap::parse_xml_file(std::istream& stream)
{
    xml_parser_.load_file_xml(stream);
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

void ValueMap::write_xml()
{
    xml_parser_.write();
}

// Accessors specializations
template <> void ValueMap::set(hash_t name, uint32_t value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(std::to_string(value).c_str());
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    uints_[name] = value;
}
template <> bool ValueMap::get(hash_t name, uint32_t& destination)
{
    auto it = uints_.find(name);
    if(it != uints_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> void ValueMap::set(hash_t name, int32_t value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(std::to_string(value).c_str());
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    ints_[name] = value;
}
template <> bool ValueMap::get(hash_t name, int32_t& destination)
{
    auto it = ints_.find(name);
    if(it != ints_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> void ValueMap::set(hash_t name, float value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(std::to_string(value).c_str());
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    floats_[name] = value;
}
template <> bool ValueMap::get(hash_t name, float& destination)
{
    auto it = floats_.find(name);
    if(it != floats_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> void ValueMap::set(hash_t name, math::vec2 value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(wcore::to_string(value).c_str());
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    vec2s_[name] = value;
}
template <> bool ValueMap::get(hash_t name, math::vec2& destination)
{
    auto it = vec2s_.find(name);
    if(it != vec2s_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> void ValueMap::set(hash_t name, math::vec3 value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(wcore::to_string(value).c_str());
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    vec3s_[name] = value;
}
template <> bool ValueMap::get(hash_t name, math::vec3& destination)
{
    auto it = vec3s_.find(name);
    if(it != vec3s_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> void ValueMap::set(hash_t name, const char* value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(value);
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    strings_[name] = value;
}
template <> void ValueMap::set(hash_t name, char* value, bool set_dom)
{
    strings_[name] = value;
}

template <> bool ValueMap::get(hash_t name, std::string& destination)
{
    auto it = strings_.find(name);
    if(it != strings_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}


template <> void ValueMap::set(hash_t name, std::reference_wrapper<const fs::path> value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(value.get().string().c_str());
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    paths_[name] = value;
}

template <> bool ValueMap::get(hash_t name, fs::path& destination)
{
    auto it = paths_.find(name);
    if(it != paths_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> void ValueMap::set(hash_t name, bool value, bool set_dom)
{
    if(set_dom)
    {
        char* value_str = xml_parser_.allocate_string(value?"true":"false");
        dom_locations_.at(name)->first_attribute("value")->value(value_str);
    }
    bools_[name] = value;
}
template <> bool ValueMap::get(hash_t name, bool& destination)
{
    auto it = bools_.find(name);
    if(it != bools_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

#ifdef __DEBUG__
void ValueMap::debug_display_content()
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

    std::cout << "--vec2--" << std::endl;
    for(auto&& [key, value]: vec2s_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--vec3--" << std::endl;
    for(auto&& [key, value]: vec3s_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--strings--" << std::endl;
    for(auto&& [key, value]: strings_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--paths--" << std::endl;
    for(auto&& [key, value]: paths_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--bools--" << std::endl;
    for(auto&& [key, value]: bools_)
        std::cout << "    " << key << " -> " << value << std::endl;
}
#endif

} // namespace wcore
