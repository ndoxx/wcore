#ifndef HEIGHT_MAP_H
#define HEIGHT_MAP_H

#include <functional>
#include <cassert>

#include "math3d.h"

namespace wcore
{

class HeightMap
{
public:
    HeightMap(uint32_t width, uint32_t length, float height=0.0f, float scale=1.0f);
    HeightMap(const HeightMap& hm);
    ~HeightMap();

    inline uint32_t get_width() const       { return width_; }
    inline uint32_t get_length() const      { return length_; }

    inline void set_width(uint32_t width)   { width_ = width; }
    inline void set_length(uint32_t length) { length_ = length; }

    inline float get_height(uint32_t xx, uint32_t zz) const;
    inline void set_height(uint32_t xx, uint32_t zz, float val);
    inline float get_height_clamped(uint32_t xx, uint32_t zz) const;

    inline void set_scale(float scale) { scale_ = scale; }
    inline float get_scale() const { return scale_; }

    float get_height(const math::vec2& pos) const;

    // File IO
    void export_data(const std::string& file);

    // Visitors
    // Apply functor func to 4 principal neighbors of (xx,zz)
    void traverse_4_neighbors(uint32_t xx, uint32_t zz, std::function<void(math::vec2, float)> func);
    // Apply functor func to 8 neighbors of (xx,zz)
    void traverse_8_neighbors(uint32_t xx, uint32_t zz, std::function<void(math::vec2, float)> func);
    // Traverse all nodes
    void traverse(std::function<void(math::vec2, float&)> func);

    inline float& operator[](uint32_t index)
    {
        assert(index<width_*length_ && "[HeightMap] index out of bounds.");
        return heights_[index];
    }

protected:
    static inline float interpolate(const math::vec3& p1,
                                    const math::vec3& p2,
                                    const math::vec3& p3,
                                    const math::vec2& pos);

private:
    uint32_t width_;   //M
    uint32_t length_;  //N
    float scale_;
    float* heights_;
};

inline float HeightMap::get_height(uint32_t xx, uint32_t zz) const
{
    assert(xx<width_  && "xx: Index out of bound in get_mesh_height().");
    assert(zz<length_ && "zz: Index out of bound in get_mesh_height().");
    return heights_[xx*length_+zz];
}

inline float HeightMap::get_height_clamped(uint32_t xx, uint32_t zz) const
{
    if(xx>width_-1)
        xx = width_-1;
    if(zz>length_-1)
        zz = length_-1;
    return heights_[xx*length_+zz];
}

inline void HeightMap::set_height(uint32_t xx, uint32_t zz, float val)
{
    assert(xx<width_  && "xx: Index out of bound in get_mesh_height().");
    assert(zz<length_ && "jj: Index out of bound in get_mesh_height().");
    heights_[xx*length_+zz] = val;
}

inline float HeightMap::interpolate(const math::vec3& p1,
                                    const math::vec3& p2,
                                    const math::vec3& p3,
                                    const math::vec2& pos)
{
    // Compute barycentric coordinates of pos
    float det = (p2.z() - p3.z()) * (p1.x() - p3.x()) + (p3.x() - p2.x()) * (p1.z() - p3.z());
    float l1 = ((p2.z() - p3.z()) * (pos.x() - p3.x()) + (p3.x() - p2.x()) * (pos.y() - p3.z())) / det;
    float l2 = ((p3.z() - p1.z()) * (pos.x() - p3.x()) + (p1.x() - p3.x()) * (pos.y() - p3.z())) / det;
    float l3 = 1.0f - l1 - l2;
    // Return interpolated value at pos using barycentric coordinates
    return l1 * p1.y() + l2 * p2.y() + l3 * p3.y();
}

}

#endif // HEIGHT_MAP_H
