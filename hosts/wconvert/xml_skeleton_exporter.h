#ifndef XML_SKELETTON_EXPORTER_H
#define XML_SKELETTON_EXPORTER_H

#include <filesystem>

#include "model_data.h"

namespace wconvert
{

namespace fs = std::filesystem;

class XMLSkeletonExporter
{
public:
    XMLSkeletonExporter();
    ~XMLSkeletonExporter();

    bool export_skeleton(const AnimatedModelInfo& model_info);

private:
    fs::path exportdir_; // Export directory for meshes and skelettons

};

} // namespace wconvert

#endif
