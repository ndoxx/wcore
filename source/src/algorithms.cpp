#include "algorithms.h"
#include "math3d.h"

namespace wcore
{
namespace math
{

// Previous power of 2 of x
uint32_t pp2(uint32_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x - (x >> 1);
}

// Next power of 2 of x
uint32_t np2(uint32_t x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

void compute_extent(const std::vector<math::vec3>& vertices, float extent[6])
{
    extent[0] = std::numeric_limits<float>::max();
    extent[1] = -std::numeric_limits<float>::max();
    extent[2] = std::numeric_limits<float>::max();
    extent[3] = -std::numeric_limits<float>::max();
    extent[4] = std::numeric_limits<float>::max();
    extent[5] = -std::numeric_limits<float>::max();

    for(uint32_t ii=0; ii<vertices.size(); ++ii)
    {
        // OBB vertices
        const vec3& vertex = vertices[ii];

        extent[0] = fmin(extent[0], vertex.x());
        extent[1] = fmax(extent[1], vertex.x());
        extent[2] = fmin(extent[2], vertex.y());
        extent[3] = fmax(extent[3], vertex.y());
        extent[4] = fmin(extent[4], vertex.z());
        extent[5] = fmax(extent[5], vertex.z());
    }
}

void compute_extent(const std::array<math::vec3, 8>& vertices, float extent[6])
{
    extent[0] = std::numeric_limits<float>::max();
    extent[1] = -std::numeric_limits<float>::max();
    extent[2] = std::numeric_limits<float>::max();
    extent[3] = -std::numeric_limits<float>::max();
    extent[4] = std::numeric_limits<float>::max();
    extent[5] = -std::numeric_limits<float>::max();

    for(uint32_t ii=0; ii<8; ++ii)
    {
        // OBB vertices
        const vec3& vertex = vertices[ii];

        extent[0] = fmin(extent[0], vertex.x());
        extent[1] = fmax(extent[1], vertex.x());
        extent[2] = fmin(extent[2], vertex.y());
        extent[3] = fmax(extent[3], vertex.y());
        extent[4] = fmin(extent[4], vertex.z());
        extent[5] = fmax(extent[5], vertex.z());
    }
}

} // namespace math
} // namespace wcore
