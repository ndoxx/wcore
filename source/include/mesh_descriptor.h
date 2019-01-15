#ifndef MESH_DESCRIPTOR_H
#define MESH_DESCRIPTOR_H

#include "wtypes.h"

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

} // namespace wcore

MAKE_HASHABLE(wcore::IcosphereProps, t.density)

#endif // MESH_DESCRIPTOR_H
