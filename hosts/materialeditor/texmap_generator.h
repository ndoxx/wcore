#ifndef NORMAL_GENERATOR_H
#define NORMAL_GENERATOR_H

#include <QImage>

namespace medit
{
namespace generator
{

enum FilterType
{
    SOBEL,
    SCHARR
};

struct NormalGenOptions
{
    FilterType filter = FilterType::SOBEL;
    float invert_r = 1.f; // not inverted: 1, inverted: -1
    float invert_g = 1.f; // not inverted: 1, inverted: -1
    float invert_h = 1.f; // not inverted: 1, inverted: -1
    float level = 7.f;
    float strength = 1.17f;
};

void normal_from_depth(const QImage& depth_map, QImage& normal_map, const NormalGenOptions& options);

} // namespace generator
} // namespace medit

#endif // NORMAL_GENERATOR_H
