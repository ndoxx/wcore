#ifndef COLORS_H
#define COLORS_H

#include "math3d.h"

namespace wcore
{
namespace color
{

extern math::vec3 rgb2hsl(const math::vec3& rgb_color);
extern math::vec3 hsl2rgb(const math::vec3& hsl_color);

inline math::vec3 rgb2hsl(float r, float g, float b)
{
    return rgb2hsl(math::vec3(r,g,b));
}

inline math::vec3 hsl2rgb(float h, float s, float l)
{
    return hsl2rgb(math::vec3(h,s,l));
}

} // namespace color
} // namsepace wcore

#endif // COLORS_H
