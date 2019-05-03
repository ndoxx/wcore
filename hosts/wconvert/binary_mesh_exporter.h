#ifndef BINARY_MESH_EXPORTER_H
#define BINARY_MESH_EXPORTER_H

#include <filesystem>

#include "animated_model_data.h"

namespace wconvert
{

namespace fs = std::filesystem;

class BinaryMeshExporter
{
public:
    BinaryMeshExporter();
    ~BinaryMeshExporter();

    bool export_mesh(const ModelInfo& model_info);

private:
    fs::path exportdir_; // Export directory for meshes and skelettons
};

} // namespace wconvert

#endif
