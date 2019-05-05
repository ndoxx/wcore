#ifndef STATIC_MODEL_IMPORTER_H
#define STATIC_MODEL_IMPORTER_H

#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "model_data.h"

namespace wconvert
{

namespace fs = std::filesystem;

class StaticModelImporter
{
public:
    StaticModelImporter();
    ~StaticModelImporter();

    bool load_model(const std::string& filename, StaticModelInfo& model_info);

private:
    bool read_mesh(const aiMesh* pmesh,
                   StaticModelInfo& model_info);

private:
    Assimp::Importer importer_;
    const aiScene* pscene_;

    fs::path workdir_; // Working directory for models
};

} // namespace wconvert

#endif
