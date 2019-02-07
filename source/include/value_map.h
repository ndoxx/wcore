#ifndef VALUE_MAP_H
#define VALUE_MAP_H

#include <map>

#include "wtypes.h"
#include "math3d.h"
#include "xml_parser.h"

namespace wcore
{

class ValueMap
{
public:
    // Generic accessors for values in maps
    template <typename T> void set(hash_t name, T value, bool set_dom=false);
    template <typename T> bool get(hash_t name, T& destination);
    // Test a boolean flag quickly
    inline bool is(hash_t name);
    // Set root directory path for path map to work
    inline void set_root_directory(const fs::path& path) { root_path_ = path; }

    // Read an XML file into the different maps
    void parse_xml_file(const fs::path& path);
    // Write to XML file
    void write_xml();

protected:
    // Recursive method for XML data hierarchy exploration
    void retrieve_configuration(rapidxml::xml_node<>* node,
                                const std::string& name_chain);
    // Parse a leaf value node in DOM
    hash_t parse_xml_property(rapidxml::xml_node<>* node,
                              const std::string& name_chain);

protected:
    // Configuration key/values
    std::map<hash_t, uint32_t>    uints_;
    std::map<hash_t, int32_t>     ints_;
    std::map<hash_t, float>       floats_;
    std::map<hash_t, std::string> strings_;
    std::map<hash_t, bool>        bools_;
    std::map<hash_t, fs::path>    paths_;
    std::map<hash_t, math::vec2>  vec2s_;
    std::map<hash_t, math::vec3>  vec3s_;

    XMLParser xml_parser_;
    std::map<hash_t, rapidxml::xml_node<>*> dom_locations_;
    fs::path root_path_;
};


inline bool ValueMap::is(hash_t name)
{
    auto it = bools_.find(name);
    if(it != bools_.end())
    {
        return it->second;
    }
    return false;
}

// Accessors specializations
template <> void ValueMap::set(hash_t name, uint32_t value, bool set_dom);
template <> bool ValueMap::get(hash_t name, uint32_t& destination);
template <> void ValueMap::set(hash_t name, int32_t value, bool set_dom);
template <> bool ValueMap::get(hash_t name, int32_t& destination);
template <> void ValueMap::set(hash_t name, float value, bool set_dom);
template <> bool ValueMap::get(hash_t name, float& destination);
template <> void ValueMap::set(hash_t name, math::vec2 value, bool set_dom);
template <> bool ValueMap::get(hash_t name, math::vec2& destination);
template <> void ValueMap::set(hash_t name, math::vec3 value, bool set_dom);
template <> bool ValueMap::get(hash_t name, math::vec3& destination);
template <> void ValueMap::set(hash_t name, const char* value, bool set_dom);
template <> void ValueMap::set(hash_t name, char* value, bool set_dom);
template <> bool ValueMap::get(hash_t name, std::string& destination);
template <> void ValueMap::set(hash_t name, std::reference_wrapper<const fs::path> value, bool set_dom);
template <> bool ValueMap::get(hash_t name, fs::path& destination);
template <> void ValueMap::set(hash_t name, bool value, bool set_dom);
template <> bool ValueMap::get(hash_t name, bool& destination);

} // namespace wcore

#endif // VALUE_MAP_H
