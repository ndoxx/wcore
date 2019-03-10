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
    SCHARR,
    OPTIMAL_SCHARR
};

struct NormalGenOptions
{
    FilterType filter = FilterType::SOBEL;
    float invert_r = 1.f; // not inverted: 1, inverted: -1
    float invert_g = 1.f; // not inverted: 1, inverted: -1
    float invert_h = 1.f; // not inverted: 1, inverted: -1
    float level = 7.f;
    float strength = 1.17f;
    float sigma = 0.0f;
};

// Generate a normal map from a depth/height map using edge detection
void normal_from_depth(const QImage& depth_map, QImage& normal_map, const NormalGenOptions& options);
// Blur/sharpen an image using a separable filter
void blur_sharp(QImage& img, float sigma);

} // namespace generator
} // namespace medit

#endif // NORMAL_GENERATOR_H
