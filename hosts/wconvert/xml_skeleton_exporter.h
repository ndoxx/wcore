#ifndef XML_SKELETTON_EXPORTER_H
#define XML_SKELETTON_EXPORTER_H

#include <filesystem>

#include "animated_model_data.h"

#include "xml_utils.hpp"
#include "vendor/rapidxml/rapidxml_print.hpp"

namespace wconvert
{

namespace fs = std::filesystem;

class XMLSkeletonExporter
{
public:
    XMLSkeletonExporter();
    ~XMLSkeletonExporter();

    bool export_skeleton(const ModelInfo& model_info);

private:
    fs::path exportdir_; // Export directory for meshes and skelettons

};

} // namespace wconvert

#endif
