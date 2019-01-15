#include "mesh_descriptor.h"

namespace wcore
{

void IcosphereProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "Density", density);
}

hash_t IcosphereProps::hash()
{
    return 0; //TODO
}

} // namespace wcore
