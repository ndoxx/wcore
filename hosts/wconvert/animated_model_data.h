#ifndef ANIMATED_MODEL_DATA_H
#define ANIMATED_MODEL_DATA_H

#include <string>
#include <vector>

#include "tree.hpp"
#include "math3d.h"
#include "vertex_format.h"

namespace wconvert
{

struct BoneInfo
{
    std::string name;
    wcore::math::mat4 offset_matrix;
};

struct MeshInfo
{
    std::vector<wcore::VertexAnim> vertices;
    std::vector<uint32_t> indices;
};

struct ModelInfo
{
    std::string model_name;
    wcore::math::mat4 root_transform;
    wcore::Tree<BoneInfo> bone_hierarchy;
    MeshInfo vertex_data;
};

} // namespace wconvert

#endif
