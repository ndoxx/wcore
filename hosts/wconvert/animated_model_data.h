#ifndef ANIMATED_MODEL_DATA_H
#define ANIMATED_MODEL_DATA_H

#include <string>

#include "tree.hpp"
#include "math3d.h"

namespace wconvert
{

struct BoneInfo
{
    std::string name;
    wcore::math::mat4 offset_matrix;
};

struct ModelInfo
{
    std::string model_name;
    wcore::Tree<BoneInfo> bone_hierarchy;
    wcore::math::mat4 root_transform;
};

} // namespace wconvert

#endif
