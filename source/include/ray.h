#ifndef RAY_H
#define RAY_H

#include "math3d.h"

namespace wcore
{

struct Ray
{
    Ray(const math::vec3& origin,
        const math::vec3& end):
    origin_w(origin),
    end_w(end),
    direction((end_w-origin_w).normalized()){}

    math::vec3 origin_w;
    math::vec3 end_w;
    math::vec3 direction;
};

} // namespace wcore


#endif // RAY_H
