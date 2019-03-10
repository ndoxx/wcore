#ifndef NORMAL_GENERATOR_H
#define NORMAL_GENERATOR_H

#include <QImage>

namespace medit
{
namespace normal
{

enum FilterType
{
    SOBEL,
    SCHARR
};

void generate_from_depth(const QImage& depth_map, QImage& normal_map, FilterType filter=FilterType::SOBEL);

} // namespace normal
} // namespace medit

#endif // NORMAL_GENERATOR_H
