#include <fstream>

#include "binary_mesh_exporter.h"
#include "wesh_loader.h"
#include "config.h"
#include "logger.h"

using namespace wcore;

namespace wconvert
{

BinaryMeshExporter::BinaryMeshExporter()
{
    wcore::CONFIG.get("root.folders.model"_h, exportdir_);
}

BinaryMeshExporter::~BinaryMeshExporter()
{

}

bool BinaryMeshExporter::export_mesh(const ModelInfo& model_info)
{
    std::string filename(model_info.model_name + ".wesh");

    DLOGN("<i>Exporting</i> mesh to:", "wconvert");
    DLOGI("<p>" + filename + "</p>", "wconvert");

    std::ofstream stream(exportdir_ / filename, std::ios::out | std::ios::binary);

    wcore::WeshLoader wesh;
    wesh.write(stream,
               model_info.vertex_data.vertices,
               model_info.vertex_data.indices);

    stream.close();
    return true;
}


} // namespace wconvert
