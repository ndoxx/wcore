#include <fstream>

#include "binary_mesh_exporter.h"
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

    const auto& vertices = model_info.vertex_data.vertices;
    const auto& indices  = model_info.vertex_data.indices;

    size_t vsize = vertices.size();
    size_t isize = indices.size();

    std::ofstream fout(exportdir_ / filename, std::ios::out | std::ios::binary);
    // Write vertex data size
    fout.write(reinterpret_cast<const char*>(&vsize), sizeof(vsize));
    // Write vertex data
    fout.write(reinterpret_cast<const char*>(&vertices[0]), vertices.size()*sizeof(VertexAnim));
    // Write index data size
    fout.write(reinterpret_cast<const char*>(&isize), sizeof(isize));
    // Write index data
    fout.write(reinterpret_cast<const char*>(&indices[0]), indices.size()*sizeof(uint32_t));

    fout.close();
    return true;
}


} // namespace wconvert
