#ifndef TERRAIN_COMMON_H
#define TERRAIN_COMMON_H

#include <vector>

namespace wcore
{

struct TerrainPatchDescriptor
{
    uint32_t chunk_size;
    uint32_t chunk_index;
    uint32_t chunk_x;
    uint32_t chunk_z;
    float lattice_scale;
    float texture_scale;
    float height;
    rapidxml::xml_node<>* generator_node;
    rapidxml::xml_node<>* height_modifier_node;
    std::vector<rapidxml::xml_node<>*> material_nodes;
};

} // namespace wcore

#endif // TERRAIN_COMMON_H
