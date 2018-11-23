#ifndef XML_UTILS_H
#define XML_UTILS_H

#include <cstring>
#include <fstream>
#include <sstream>
#include <functional>
#include <memory>
#include <vector>

#include "rapidxml/rapidxml.hpp"
#include "math3d.h"

namespace wcore
{
namespace xml
{

template <typename T>
bool str_val(const char* value, T& result)
{
    std::istringstream iss(value);
    return !(iss >> result).fail();
}

template <typename T>
void parse_attribute(rapidxml::xml_node<>* node, const char* name, std::function<void(T value)> exec)
{
    T value;
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr) return;

    if(str_val(pAttr->value(), value))
        exec(value);
}

template <typename T>
void parse_node(rapidxml::xml_node<>* parent, const char* name, std::function<void(T value)> exec)
{
    rapidxml::xml_node<>* leaf_node = parent->first_node(name);
    if(!leaf_node) return;

    T value;
    bool success = str_val(leaf_node->value(), value);
    if(success)
        exec(value);
}

template <typename T>
bool parse_attribute(rapidxml::xml_node<>* node, const char* name, T& destination)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return false;
    return str_val(node->first_attribute(name)->value(), destination);
}

template <typename T>
bool parse_node(rapidxml::xml_node<>* parent, const char* leaf_name, T& destination)
{
    rapidxml::xml_node<>* leaf_node = parent->first_node(leaf_name);
    if(!leaf_node)
        return false;

    return str_val(leaf_node->value(), destination);
}

bool parse_attribute(rapidxml::xml_node<>* node, const char* name, std::string& destination);
bool parse_node(rapidxml::xml_node<>* parent, const char* leaf_name, std::string& destination);

// Full specializations
template <>
bool str_val<math::vec<2> >(const char* value, math::vec<2>& result);

template <>
bool str_val<math::vec<3> >(const char* value, math::vec<3>& result);

template <>
bool str_val<math::vec<2,uint32_t> >(const char* value, math::vec<2,uint32_t>& result);

template <>
bool str_val<math::vec<3,uint32_t> >(const char* value, math::vec<3,uint32_t>& result);

template <>
bool str_val<bool>(const char* value, bool& result);

template <>
void parse_node<const char*>(rapidxml::xml_node<>* parent, const char* name, std::function<void(const char* value)> exec);


// Specific to WCore
template <typename T>
bool parse_property(rapidxml::xml_node<>* parent, const char* prop_name, T& destination)
{
    for (rapidxml::xml_node<>* prop=parent->first_node("Prop");
         prop; prop=prop->next_sibling("Prop"))
    {
        rapidxml::xml_attribute<>* pAttr = prop->first_attribute("name");
        if(!pAttr) continue;
        const char* propertyName = pAttr->value();

        if(!strcmp(propertyName, prop_name))
            return xml::str_val(prop->value(), destination);
    }
    return false;
}

// UNUSED
class IProperty
{
protected:
    std::string name_;

public:
    explicit IProperty(const std::string& name):
        name_(name){}
    virtual ~IProperty(){}
};

template <typename T>
class Property: public IProperty
{
public:
    T& destination;

    Property(const std::string& name, T& destination):
        IProperty(name),
        destination(destination){}
};

typedef std::vector<std::shared_ptr<IProperty>> PropertyList;

}
}
#endif // XML_UTILS_H
