#include <random>

#include "colors.h"

namespace wcore
{
namespace color
{

static const float SQ3_2 = 0.866025404f;
static const math::vec3 W(0.2126, 0.7152, 0.0722);

math::vec3 rgb2hsl(const math::vec3& rgb_color)
{
    // Auxillary Cartesian chromaticity coordinates
    float a = 0.5f*(2.0f*rgb_color.r()-rgb_color.g()-rgb_color.b());
    float b = SQ3_2*(rgb_color.g()-rgb_color.b());
    // Hue
    float h = atan2(b, a);
    // Chroma
    float c = sqrt(a*a+b*b);
    // We choose Luma Y_709 (for sRGB primaries) as a definition for Lightness
    float l = rgb_color.dot(W);
    // Saturation
    float s = (l==1.0f) ? 0.0f : c/(1.0f-fabs(2*l-1));
    // Just convert hue from radians in [-pi/2,pi/2] to [0,1]
    return math::vec3(h/M_PI+0.5f,s,l);
}

math::vec3 hsl2rgb(const math::vec3& hsl_color)
{
    // Chroma
    float c = (1.0f-fabs(2.0f*hsl_color.z()-1.0f))*hsl_color.y();
    // Intermediate
    float Hp = hsl_color.x()*6.0f; // Map to degrees -> *360°, divide by 60° -> *6
    float x = c*(1.0f-fabs(fmod(Hp,2) - 1.0f));
    uint8_t hn = uint8_t(floor(Hp));

    // Lightness offset
    float m = hsl_color.z()-0.5f*c;

    // Project to RGB cube
    switch(hn)
    {
        case 0:
            return math::vec3(c+m,x+m,m);
        case 1:
            return math::vec3(x+m,c+m,m);
        case 2:
            return math::vec3(m,c+m,x+m);
        case 3:
            return math::vec3(m,x+m,c+m);
        case 4:
            return math::vec3(x+m,m,c+m);
        case 5:
            return math::vec3(c+m,m,x+m);
        default:
            return math::vec3(m,m,m);
    }
}

math::i32vec3 rgbfloat2rgbuint(const math::vec3& rgb_color)
{
    return math::i32vec3(uint32_t(rgb_color.r()*255),
                         uint32_t(rgb_color.g()*255),
                         uint32_t(rgb_color.b()*255));
}


math::vec3 random_color(unsigned long long seed,
                        float saturation,
                        float lightness)
{
    std::mt19937 generator(seed);
    std::uniform_real_distribution<float> distribution(0.f,1.f);

    math::vec3 hsl(distribution(generator),
                   saturation,
                   lightness);
    return hsl2rgb(hsl);
}


} // namespace color
} // namespace wcore
