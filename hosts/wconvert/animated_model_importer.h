#ifndef ANIMATED_MODEL_IMPORTER_H
#define ANIMATED_MODEL_IMPORTER_H

#include <filesystem>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "tree.hpp"
#include "math3d.h"

namespace wconvert
{

namespace fs = std::filesystem;

struct BoneInfo
{
    aiMatrix4x4 offset_matrix;
};

struct Bone
{
    std::string name;
    wcore::math::mat4 offset_matrix;
};

class AnimatedModelImporter
{
public:
    AnimatedModelImporter();
    ~AnimatedModelImporter();

    bool load_model(const std::string& filename);

private:
    int read_bone_hierarchy(const aiNode* pnode,
                            wcore::Tree<Bone>& bone_hierarchy);

private:
    Assimp::Importer importer_;
    const aiScene* pscene_;

    fs::path workdir_; // Working directory for models

    std::unordered_map<std::string, uint32_t> bone_map_; // Associate bone name to bone index
    std::vector<BoneInfo> bone_info_;
    uint32_t n_bones_;

    wcore::Tree<Bone> bone_hierarchy_;
};

} // namespace wconvert

#endif
