#ifndef TERRAIN_PATCH_H
#define TERRAIN_PATCH_H

#include <set>

#include "model.h"

namespace wcore
{

class HeightMap;

/*
    A TerrainChunk is a piece of terrain, the size of
    a chunk. It's basically a Model, with additional
    properties.
*/
class TerrainChunk : public Model
{
private:
    HeightMap* heightmap_;
    //float lattice_scale_;
    //float texture_scale_;

    static uint32_t chunk_size_;

public:
    static inline void set_chunk_size(uint32_t value) { chunk_size_ = value; }
    static inline uint32_t get_chunk_size()           { return chunk_size_; }

    TerrainChunk(HeightMap* phm,
                 Material* pmat,
                 float latticeScale=1.0f,
                 float textureScale=1.0f);

    virtual ~TerrainChunk();

    inline const HeightMap& get_heightmap() const { return *heightmap_; }

    // Terrain geometry accessors
    inline Vertex3P3N3T2U& north(uint32_t index);
    inline Vertex3P3N3T2U& south(uint32_t index);
    inline Vertex3P3N3T2U& east(uint32_t index);
    inline Vertex3P3N3T2U& west(uint32_t index);
};

inline Vertex3P3N3T2U& TerrainChunk::east(uint32_t index)
{
    return (*pmesh_)[(chunk_size_-1)*chunk_size_+index];
}

inline Vertex3P3N3T2U& TerrainChunk::west(uint32_t index)
{
    return (*pmesh_)[index];
}

inline Vertex3P3N3T2U& TerrainChunk::south(uint32_t index)
{
    return (*pmesh_)[index*chunk_size_];
}

inline Vertex3P3N3T2U& TerrainChunk::north(uint32_t index)
{
    return (*pmesh_)[index*chunk_size_ + chunk_size_-1];
}

class Scene;
namespace terrain
{
// Access loaded neighbor chunks and fix this terrain chunk's edges normals and tangents
extern void stitch_terrain_edges(Scene* pscene,
                                 std::shared_ptr<TerrainChunk> terrain,
                                 uint32_t chunk_index,
                                 uint32_t chunk_size);
}

}

#endif // TERRAIN_PATCH_H
