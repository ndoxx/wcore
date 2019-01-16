#include <filesystem>

#include "model_factory.h"
#include "material_factory.h"
#include "surface_mesh_factory.h"
#include "io_utils.h"
#include "xml_utils.hpp"

namespace fs = std::filesystem;

namespace wcore
{

ModelFactory::ModelFactory(const char* assetfile):
mesh_factory_(new SurfaceMeshFactory()),
material_factory_(new MaterialFactory())
{
    parse_asset_file(assetfile);
    rapidxml::xml_node<>* materials_node = xml_parser_.get_root()->first_node("Materials");
    rapidxml::xml_node<>* meshes_node = xml_parser_.get_root()->first_node("Meshes");
    material_factory_->retrieve_asset_descriptions(materials_node);
    mesh_factory_->retrieve_asset_descriptions(meshes_node);
}

ModelFactory::~ModelFactory()
{

}

void ModelFactory::parse_asset_file(const char* xmlfile)
{
    fs::path file_path(io::get_file(H_("root.folders.level"), xmlfile));
    xml_parser_.load_file_xml(file_path);
}


} // namespace wcore

