#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <cstdint>
#include <vector>
#include <array>

namespace wcore
{
namespace math
{

template <unsigned N, typename T> class vec;

// Previous power of 2 of x
extern uint32_t pp2(uint32_t x);
// Next power of 2 of x
extern uint32_t np2(uint32_t x);

template<typename T>
inline T clamp(const T &a, const T &min, const T &max)
{
    if (a < min) return min;
    else if (a > max) return max;
    else return a;
}

extern void compute_extent(const std::vector<math::vec<3,float>>& vertices, float extent[6]);
extern void compute_extent(const std::array<math::vec<3,float>, 8>& vertices, float extent[6]);

} // namespace math
} // namespace wcore

#endif // ALGORITHMS_H
