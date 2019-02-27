#include "terrain_patch.h"
#include "mesh_factory.h"
#include "surface_mesh.h"
#include "height_map.h"
#include "material.h"
#include "texture.h"
#include "scene.h"

namespace wcore
{

using namespace math;

uint32_t TerrainChunk::chunk_size_ = 0;

TerrainChunk::TerrainChunk(HeightMap* phm,
                           Material* pmat,
                           float latticeScale,
                           float textureScale):
Model(static_cast<std::shared_ptr<SurfaceMesh>>(factory::make_terrain_tri_mesh(*phm,
                            latticeScale,
                            textureScale)),
      pmat),
heightmap_(phm),
alt_material_(nullptr),
splatmap_(nullptr),
use_splat_(false)
{
    is_terrain_ = true;
}

TerrainChunk::~TerrainChunk()
{
    delete heightmap_;
    if(alt_material_)
        delete alt_material_;
    if(splatmap_)
        delete splatmap_;
}

namespace terrain
{
void stitch_terrain_edges(Scene* pscene,
                          TerrainChunk& terrain,
                          uint32_t chunk_index,
                          uint32_t chunk_size)
{
    pscene->traverse_loaded_neighbor_chunks(chunk_index, [&](Chunk* chunk, wcore::NEIGHBOR location)
    {
        const TerrainChunk& nei_terrain = chunk->get_terrain_nc();
        switch(location)
        {
            case wcore::NEIGHBOR::SOUTH:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain.south(ii).normal_ = nei_terrain.north(ii).normal_;
                    terrain.south(ii).tangent_ = nei_terrain.north(ii).tangent_;
                    terrain.south(ii).position_[1] = nei_terrain.north(ii).position_[1];
                }
                break;
            }
            case wcore::NEIGHBOR::NORTH:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain.north(ii).normal_ = nei_terrain.south(ii).normal_;
                    terrain.north(ii).tangent_ = nei_terrain.south(ii).tangent_;
                    terrain.north(ii).position_[1] = nei_terrain.south(ii).position_[1];
                }
                break;
            }
            case wcore::NEIGHBOR::EAST:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain.east(ii).normal_ = nei_terrain.west(ii).normal_;
                    terrain.east(ii).tangent_ = nei_terrain.west(ii).tangent_;
                    terrain.east(ii).position_[1] = nei_terrain.west(ii).position_[1];
                }
                break;
            }
            case wcore::NEIGHBOR::WEST:
            {
                for(uint32_t ii=0; ii<chunk_size; ++ii)
                {
                    terrain.west(ii).normal_ = nei_terrain.east(ii).normal_;
                    terrain.west(ii).tangent_ = nei_terrain.east(ii).tangent_;
                    terrain.west(ii).position_[1] = nei_terrain.east(ii).position_[1];
                }
                break;
            }
        }
    });
}

} // namespace terrain
} // namespace wcore
