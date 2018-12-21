#ifndef RAY_H
#define RAY_H

#include "math3d.h"

namespace wcore
{

struct Ray
{
    Ray(const math::vec3& start_world,
        const math::vec3& end_world):
    start_world(start_world),
    end_world(end_world){}

    Ray(math::vec3&& start_world,
        math::vec3&& end_world):
    start_world(std::move(start_world)),
    end_world(std::move(end_world)){}

    math::vec3 start_world;
    math::vec3 end_world;
};

} // namespace wcore


#endif // RAY_H
