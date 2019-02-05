#include "xml_utils.hpp"
#include "wtypes.h"
#include "math3d.h"
#include "transformation.h"
#include "logger.h"

namespace wcore
{

using namespace math;
using namespace rapidxml;

namespace xml
{

template <>
bool str_val<math::vec<2> >(const char* value, math::vec<2>& result)
{
    return sscanf(value, "(%f,%f)", &result[0], &result[1]) > 0;
}

template <>
bool str_val<math::vec<3> >(const char* value, math::vec<3>& result)
{
    return sscanf(value, "(%f,%f,%f)", &result[0], &result[1], &result[2]) > 0;
}

template <>
bool str_val<math::vec<2,uint32_t> >(const char* value, math::vec<2,uint32_t>& result)
{
    return sscanf(value, "(%d,%d)", &result[0], &result[1]) > 0;
}

template <>
bool str_val<math::vec<3,uint32_t> >(const char* value, math::vec<3,uint32_t>& result)
{
    return sscanf(value, "(%d,%d,%d)", &result[0], &result[1], &result[2]) > 0;
}

template <>
bool str_val<bool>(const char* value, bool& result)
{
    result = !strcmp(value, "true");
    return true;
}

template <>
void parse_node<const char*>(xml_node<>* parent, const char* name, std::function<void(const char* value)> exec)
{
    xml_node<>* leaf_node = parent->first_node(name);
    if(!leaf_node)
        return;

    exec(leaf_node->value());
}

bool parse_attribute(xml_node<>* node, const char* name, std::string& destination)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return false;

    destination = pAttr->value();
    return true;
}

hash_t parse_attribute_h(rapidxml::xml_node<>* node, const char* name)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return 0;

    return H_(node->first_attribute(name)->value());
}

bool parse_node(xml_node<>* parent, const char* leaf_name, std::string& destination)
{
    xml_node<>* leaf_node = parent->first_node(leaf_name);
    if(!leaf_node)
        return false;

    destination = leaf_node->value();
    return true;
}

} // namespace xml
} // namespace wcore
