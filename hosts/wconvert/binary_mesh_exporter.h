#ifndef BINARY_MESH_EXPORTER_H
#define BINARY_MESH_EXPORTER_H

#include <filesystem>

#include "model_data.h"

namespace wconvert
{

namespace fs = std::filesystem;

class BinaryMeshExporter
{
public:
    BinaryMeshExporter();
    ~BinaryMeshExporter();

    bool export_mesh(const AnimatedModelInfo& model_info);
    bool export_mesh(const StaticModelInfo& model_info);

private:
    fs::path exportdir_; // Export directory for meshes and skelettons
};

} // namespace wconvert

#endif
