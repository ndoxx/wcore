#ifndef MODEL_DATA_H
#define MODEL_DATA_H

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

struct AnimatedMeshInfo
{
    std::vector<wcore::VertexAnim> vertices;
    std::vector<uint32_t> indices;
};

struct AnimatedModelInfo
{
    std::string model_name;
    wcore::math::mat4 root_transform;
    wcore::Tree<BoneInfo> bone_hierarchy;
    AnimatedMeshInfo vertex_data;
};

struct StaticMeshInfo
{
    std::vector<wcore::Vertex3P3N3T2U> vertices;
    std::vector<uint32_t> indices;
};

struct StaticModelInfo
{
    std::string model_name;
    StaticMeshInfo vertex_data;
};

} // namespace wconvert

#endif
