#include "terrain_factory.h"
#include "height_map.h"
#include "heightmap_generator.h"
#include "logger.h"

namespace wcore
{

HeightMap* TerrainFactory::make_heightmap(const TerrainPatchDescriptor& desc)
{
    // --- Heightmap creation
    // Add 1 to heightmap length for seamless terrain with hex terrain triangle mesh
    HeightMap* heightmap = new HeightMap(desc.chunk_size,
                                         desc.chunk_size+1,
                                         desc.height,
                                         desc.lattice_scale);

    // --- Heightmap generation
    // TODO
    // Parse generators first hand (instantiate) then use
    // a table to select the correct generator for current chunk.
    // Hightmap generation
    if(desc.generator_node)
    {
        std::string type;
        uint32_t seed = 0;
        xml::parse_attribute(desc.generator_node, "seed", seed);
        if(!xml::parse_attribute(desc.generator_node, "type", type))
        {
            DLOGE("[ModelFactory] Terrain Generator node must have a 'type' attribute initialized.", "parsing", Severity::CRIT);
            delete heightmap;
            return nullptr;
        }

        std::mt19937 rng;
        rng.seed(seed);
        if(!type.compare("simplex"))
            generate_simplex(*heightmap, desc, rng);
    }

    // Apply modifier stack to height map
    if(desc.height_modifier_node)
    {
        for (rapidxml::xml_node<>* modifier=desc.height_modifier_node->first_node();
             modifier; modifier=modifier->next_sibling())
        {
            if(!strcmp(modifier->name(),"Randomizer"))
                modify_randomize(*heightmap, modifier);
            else if(!strcmp(modifier->name(),"Erosion"))
                modify_erode(*heightmap, modifier);
            else if(!strcmp(modifier->name(),"Offset"))
                modify_offset(*heightmap, modifier);
        }
    }

    return heightmap;
}

void TerrainFactory::generate_simplex(HeightMap& input, const TerrainPatchDescriptor& desc, std::mt19937& rng)
{
    SimplexNoiseProps props;
    props.scale = desc.lattice_scale;
    props.parse_xml(desc.generator_node);
    props.hiBound /= desc.lattice_scale;
    // Compute starting coordinates (bottom-rightmost)
    props.startX = desc.chunk_x * (desc.chunk_size-1);
    props.startZ = desc.chunk_z * (desc.chunk_size-1);

    HeightmapGenerator::init_simplex_generator(rng);
    HeightmapGenerator::heightmap_from_simplex_noise(input, props);
}

void TerrainFactory::modify_randomize(HeightMap& input, rapidxml::xml_node<>* modifier_node)
{
    uint32_t seed, xmin, xmax, ymin, ymax;
    float height_variance;
    bool success = true;
    success &= xml::parse_attribute(modifier_node, "seed", seed);
    success &= xml::parse_attribute(modifier_node, "xmin", xmin);
    success &= xml::parse_attribute(modifier_node, "xmax", xmax);
    success &= xml::parse_attribute(modifier_node, "ymin", ymin);
    success &= xml::parse_attribute(modifier_node, "ymax", ymax);
    success &= xml::parse_attribute(modifier_node, "variance", height_variance);
    if(!success) return;

    std::mt19937 rng;
    rng.seed(seed);
    std::uniform_real_distribution<float> var_distrib(-1.0f,1.0f);

    input.traverse([&](math::vec2 pos2d, float& height)
    {
        if(pos2d.x()>=xmin && pos2d.x()<=xmax && pos2d.y()>=ymin && pos2d.y()<=ymax)
            height += height_variance * var_distrib(rng);
    });
}

void TerrainFactory::modify_erode(HeightMap& input, rapidxml::xml_node<>* modifier_node)
{
    std::string type;
    if(!xml::parse_attribute(modifier_node, "type", type))
        return;

    if(!type.compare("plateau"))
    {
        PlateauErosionProps props;
        props.parse_xml(modifier_node);
        HeightmapGenerator::erode(input, props);
    }
    else if(!type.compare("droplets"))
    {
        DropletErosionProps props;
        props.parse_xml(modifier_node);
        HeightmapGenerator::erode_droplets(input, props);
    }
}

void TerrainFactory::modify_offset(HeightMap& input, rapidxml::xml_node<>* modifier_node)
{
    float offset;
    if(!xml::parse_attribute(modifier_node, "y", offset)) return;

    offset /= input.get_scale();
    input.traverse([&](math::vec2 pos2d, float& height){ height += offset; });
}

} // namespace wcore
