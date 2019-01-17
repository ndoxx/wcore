#ifndef MESH_DESCRIPTOR_H
#define MESH_DESCRIPTOR_H

#include "wtypes.h"
#include "math3d.h"

namespace rapidxml
{
    template<class Ch> class xml_node;
}

namespace wcore
{

struct MeshDescriptor
{
    virtual void parse_xml(rapidxml::xml_node<char>* node) = 0;
};

struct IcosphereProps: public MeshDescriptor
{
    virtual void parse_xml(rapidxml::xml_node<char>* node) override;

    uint32_t density;
};

struct BoxProps: public MeshDescriptor
{
    virtual void parse_xml(rapidxml::xml_node<char>* node) override;

    math::extent_t extent;
    float texture_scale;
};

} // namespace wcore

MAKE_HASHABLE(wcore::IcosphereProps, t.density)
MAKE_HASHABLE(wcore::BoxProps, t.extent[0], t.extent[1], t.extent[2],
              t.extent[3], t.extent[4], t.extent[5], t.texture_scale)

#endif // MESH_DESCRIPTOR_H
