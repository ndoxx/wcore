#include "height_map.h"
#include "logger.h"

HeightMap::HeightMap(uint32_t width, uint32_t length, float height)
: width_(width)
, length_(length)
, scale_(1.0f)
, heights_(new float[width_*length_])
{
    for(uint32_t ii=0; ii<width_*length_; ++ii)
        heights_[ii] = height;
}

HeightMap::HeightMap(const HeightMap& hm)
: width_(hm.width_)
, length_(hm.length_)
, scale_(1.0f)
, heights_(new float[width_*length_])
{
    for(uint32_t ii=0; ii<width_*length_; ++ii)
        heights_[ii] = hm.heights_[ii];
}

HeightMap::~HeightMap()
{
    delete [] heights_;
}

void HeightMap::export_data(const std::string& file)
{
    std::ofstream out(file);
    for(uint32_t ii=0; ii<width_; ++ii)
    {
        for(uint32_t jj=0; jj<length_; ++jj)
        {
            out << get_height(ii,jj) << " ";
        }
        out << std::endl;
    }
    out.close();
}

void HeightMap::traverse_4_neighbors(uint32_t xx,
                                     uint32_t zz,
                                     std::function<void(math::vec2, float)> func)
{
    if (xx > 0)
    {
        math::vec2 pos(xx - 1, zz);
        func(pos, get_height(pos.x(),pos.y()));
    }
    if (xx < width_ - 1)
    {
        math::vec2 pos(xx + 1, zz);
        func(pos, get_height(pos.x(),pos.y()));
    }
    if (zz > 0)
    {
        math::vec2 pos(xx, zz - 1);
        func(pos, get_height(pos.x(),pos.y()));
    }
    if (zz < length_ - 1)
    {
        math::vec2 pos(xx, zz + 1);
        func(pos, get_height(pos.x(),pos.y()));
    }
}

void HeightMap::traverse_8_neighbors(uint32_t xx,
                                     uint32_t zz,
                                     std::function<void(math::vec2, float)> func)
{
    for (int ii = -1; ii <= 1; ++ii)
    {
        if (xx == 0          && ii == -1) continue;
        if (xx == width_ - 1 && ii ==  1) continue;

        for (int jj = -1; jj <= 1; ++jj)
        {
            if (zz == 0           && jj == -1) continue;
            if (zz == length_ - 1 && jj ==  1) continue;

            if (ii != 0 || jj != 0)
            {
                math::vec2 pos(xx + ii, zz + jj);
                func(pos, get_height(pos.x(),pos.y()));
            }
        }
    }
}

void HeightMap::traverse(std::function<void(math::vec2, float&)> func)
{
    // ii == xx*length_+zz
    for(uint32_t xx=0; xx<width_; ++xx)
    {
        for(uint32_t zz=0; zz<length_; ++zz)
        {
            func(math::vec2(xx,zz), heights_[xx*length_+zz]);
        }
    }
}

float HeightMap::get_height(const math::vec2& pos) const
{
    math::vec2 pos2D(pos);
    pos2D /= scale_;
    float y=0;

    // Coords in the grid (model pos of bottom left corner of the quad we're in)
    int ii = (int)floor(pos2D.x());
    int jj = (int)floor(pos2D.y());

    // If out of the mesh grid, return a default height of 0
    if(ii<0 || ii>width_-1 || jj<0 || jj>length_-1) return 0.0f;

    // Get the heights of the different quad corners
    float hbl = get_height(ii,  jj);
    float hbr = get_height(ii,  jj+1);
    float htr = get_height(ii+1,jj+1);
    float htl = get_height(ii+1,jj);

    // Transform position to inner quad coordinates (in [0 1])
    pos2D -= math::vec2(ii, jj);

    // In the current quad, in which triangle do we find ourselves?
    if (pos2D.x() <= (1-pos2D.y())) // Top Left Triangle
    {
        // Interpolate height using barycentric coordinates
        y = interpolate(math::vec3(0, hbl, 0),
                        math::vec3(1, htl, 0),
                        math::vec3(0, hbr, 1),
                        pos2D);
    }
    else                           // Bottom Right Triangle
    {
        // Interpolate height using barycentric coordinates
        y = interpolate(math::vec3(1, htl, 0),
                        math::vec3(1, htr, 1),
                        math::vec3(0, hbr, 1),
                        pos2D);
    }

    // Return correctly scaled height
    return y*scale_;
}
