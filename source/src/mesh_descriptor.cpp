#include "mesh_descriptor.h"

namespace wcore
{

void IcosphereProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "Density", density);
}

void BoxProps::parse_xml(rapidxml::xml_node<char>* node)
{
    xml::parse_node(node, "xmin", extent[0]);
    xml::parse_node(node, "xmax", extent[1]);
    xml::parse_node(node, "ymin", extent[2]);
    xml::parse_node(node, "ymax", extent[3]);
    xml::parse_node(node, "zmin", extent[4]);
    xml::parse_node(node, "zmax", extent[5]);
    if(!xml::parse_node(node, "TextureScale", texture_scale))
        texture_scale = 1.0f;
}

} // namespace wcore
