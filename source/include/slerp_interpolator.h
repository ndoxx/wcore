#ifndef SLERP_INTERPOLATOR_H
#define SLERP_INTERPOLATOR_H

#include <vector>

#include "quaternion.h"

namespace wcore
{

class SlerpInterpolator
{
public:
    SlerpInterpolator(const std::vector<float>& domain,
                      const std::vector<math::quat>& points);

    math::quat interpolate(float t);

private:
    std::vector<float>      domain_;           // Loci of control points in parameter space
    std::vector<math::quat> points_;           // Control points
};

} // namespace wcore

#endif // SLERP_INTERPOLATOR_H
