#include "terrain_patch.h"
#include "mesh_factory.h"
#include "height_map.h"
#include "scene.h"

using namespace math;

uint32_t TerrainChunk::chunk_size_ = 0;

TerrainChunk::TerrainChunk(HeightMap* phm,
                           Material* pmat,
                           float latticeScale,
                           float textureScale):
Model((Mesh<Vertex3P3N3T2U>*)factory::make_terrain_tri_mesh(*phm,
                            latticeScale,
                            textureScale),
      pmat),
heightmap_(phm)
//lattice_scale_(latticeScale),
//texture_scale_(textureScale)
{

}

TerrainChunk::~TerrainChunk()
{
    delete heightmap_;
}

namespace terrain
{
void stitch_terrain_edges(std::shared_ptr<TerrainChunk> terrain,
                          uint32_t chunk_index,
                          uint32_t chunk_size)
{
    SCENE.traverse_loaded_neighbor_chunks(chunk_index, [&](Chunk* chunk, wcore::NEIGHBOR location)
    {
        std::shared_ptr<TerrainChunk> nei_terrain = chunk->get_terrain_nc();
        switch(location)
        {
            case wcore::NEIGHBOR::SOUTH:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain->south(ii).set_normal(nei_terrain->north(ii).get_normal());
                    terrain->south(ii).set_tangent(nei_terrain->north(ii).get_tangent());
                }
                break;
            }
            case wcore::NEIGHBOR::NORTH:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain->north(ii).set_normal(nei_terrain->south(ii).get_normal());
                    terrain->north(ii).set_tangent(nei_terrain->south(ii).get_tangent());
                }
                break;
            }
            case wcore::NEIGHBOR::EAST:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain->east(ii).set_normal(nei_terrain->west(ii).get_normal());
                    terrain->east(ii).set_tangent(nei_terrain->west(ii).get_tangent());
                }
                break;
            }
            case wcore::NEIGHBOR::WEST:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain->west(ii).set_normal(nei_terrain->east(ii).get_normal());
                    terrain->west(ii).set_tangent(nei_terrain->east(ii).get_tangent());
                }
                break;
            }
        }
    });
}

}
