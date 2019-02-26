#ifndef TERRAIN_COMMON_H
#define TERRAIN_COMMON_H

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
    rapidxml::xml_node<>* material_node;
    rapidxml::xml_node<>* alt_material_node;
    rapidxml::xml_node<>* generator_node;
    rapidxml::xml_node<>* height_modifier_node;
};

} // namespace wcore

#endif // TERRAIN_COMMON_H
