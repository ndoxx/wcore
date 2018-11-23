#ifndef COLORS_H
#define COLORS_H

#include "math3d.h"

namespace wcore
{
namespace color
{

extern math::vec3 rgb2hsl(const math::vec3& rgb_color);
extern math::vec3 hsl2rgb(const math::vec3& hsl_color);
extern math::i32vec3 rgbfloat2rgbuint(const math::vec3& rgb_color);

extern math::vec3 random_color(unsigned long long seed,
                               float saturation=1.f,
                               float lightness=0.5f);

inline math::i32vec3 random_color_uint(unsigned long long seed,
                                       float saturation=1.f,
                                       float lightness=0.5f)
{
    return rgbfloat2rgbuint(random_color(seed, saturation, lightness));
}

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
