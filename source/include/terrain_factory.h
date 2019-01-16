#ifndef TERRAIN_FACTORY_H
#define TERRAIN_FACTORY_H

#include <random>

#include "wtypes.h"
#include "xml_parser.h"
#include "terrain_common.h"

namespace wcore
{

class HeightMap;
class TerrainFactory
{
public:
    HeightMap* make_heightmap(const TerrainPatchDescriptor& desc);

private:
    void generate_simplex(HeightMap& input, const TerrainPatchDescriptor& desc, std::mt19937& rng);
    void modify_randomize(HeightMap& input, rapidxml::xml_node<>* modifier_node);
    void modify_erode(HeightMap& input, rapidxml::xml_node<>* modifier_node);
    void modify_offset(HeightMap& input, rapidxml::xml_node<>* modifier_node);
};

} // namespace wcore


#endif // TERRAIN_FACTORY_H
