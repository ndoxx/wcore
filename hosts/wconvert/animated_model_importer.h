#ifndef ANIMATED_MODEL_IMPORTER_H
#define ANIMATED_MODEL_IMPORTER_H

#include <filesystem>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "animated_model_data.h"

namespace wconvert
{

namespace fs = std::filesystem;

class AnimatedModelImporter
{
public:
    AnimatedModelImporter();
    ~AnimatedModelImporter();

    bool load_model(const std::string& filename, ModelInfo& model_info);

private:
    bool read_mesh(const aiMesh* pmesh,
                   ModelInfo& model_info);
    int read_bone_hierarchy(const aiNode* pnode,
                            ModelInfo& model_info);
    void reset();

private:
    Assimp::Importer importer_;
    const aiScene* pscene_;

    fs::path workdir_; // Working directory for models

    // Internal data used to walk the Assimp structures
    std::unordered_map<std::string, uint32_t> bone_map_; // Associate bone name to bone index
    std::vector<BoneInfo> bone_info_;
    uint32_t n_bones_;
};

} // namespace wconvert

#endif
