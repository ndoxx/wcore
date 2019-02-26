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
    Material* alt_material_;
    bool use_splat_;

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
    inline Vertex3P3N3T2U& north(uint32_t index) const;
    inline Vertex3P3N3T2U& south(uint32_t index) const;
    inline Vertex3P3N3T2U& east(uint32_t index) const;
    inline Vertex3P3N3T2U& west(uint32_t index) const;

    // Splat mapping
    inline void add_alternative_material(Material* pmat);
    inline void add_splat_mat(/* */);
    inline bool has_splat_map() const;
    inline const Material& get_alternative_material() const;
};

inline Vertex3P3N3T2U& TerrainChunk::east(uint32_t index) const
{
    return (*pmesh_)[(chunk_size_-1)*chunk_size_+index];
}

inline Vertex3P3N3T2U& TerrainChunk::west(uint32_t index) const
{
    return (*pmesh_)[index];
}

inline Vertex3P3N3T2U& TerrainChunk::south(uint32_t index) const
{
    return (*pmesh_)[index*chunk_size_];
}

inline Vertex3P3N3T2U& TerrainChunk::north(uint32_t index) const
{
    return (*pmesh_)[index*chunk_size_ + chunk_size_-1];
}

inline void TerrainChunk::add_alternative_material(Material* pmat)
{
    alt_material_ = pmat;
}
inline void TerrainChunk::add_splat_mat(/* */)
{

    use_splat_ = true;
}
inline bool TerrainChunk::has_splat_map() const
{
    return use_splat_;
}
inline const Material& TerrainChunk::get_alternative_material() const
{
    if(use_splat_)
        return *alt_material_;
    else
        return *pmaterial_;
}


class Scene;
namespace terrain
{
// Access loaded neighbor chunks and fix this terrain chunk's edges normals and tangents
extern void stitch_terrain_edges(Scene* pscene,
                                 TerrainChunk& terrain,
                                 uint32_t chunk_index,
                                 uint32_t chunk_size);
}

}

#endif // TERRAIN_PATCH_H
