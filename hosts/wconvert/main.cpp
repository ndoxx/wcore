#include <iostream>
#include <filesystem>
#include <vector>
#include <cassert>

#include "config.h"
#include "file_system.h"
#include "logger.h"
#include "wesh_loader.h"
#include "vertex_format.h"
#include "animated_model_importer.h"
#include "static_model_importer.h"
#include "xml_skeleton_exporter.h"
#include "binary_mesh_exporter.h"
#include "mesh.hpp"

using namespace wcore;
namespace fs = std::filesystem;

void read_directory(const fs::path& directory, std::vector<std::string>& v)
{
    fs::directory_iterator start(directory);
    fs::directory_iterator end;
    std::transform(start, end, std::back_inserter(v), [&](const fs::directory_entry& entry)
    {
        return entry.path().filename().string();
    });
}

bool check_extension(const std::string& filename, const char* ext)
{
    return !filename.substr(filename.find('.')).compare(ext);
}

void test_read_binary(const std::string& filename, const wconvert::AnimatedModelInfo& model_info)
{
    std::string stripped_filename(filename.substr(0, filename.find('.')));
    // Test read binary mesh data
    std::ifstream stream("../res/models/" + stripped_filename + ".wesh", std::ios::in | std::ios::binary);

    std::vector<VertexAnim> vdata;
    std::vector<uint32_t> idata;

    WeshLoader wesh;
    wesh.read(stream, vdata, idata);
    stream.close();

    auto pmesh = std::make_shared<Mesh<VertexAnim>>(std::move(vdata), std::move(idata));

    assert(pmesh->get_nv() == model_info.vertex_data.vertices.size());
    assert(pmesh->get_ni() == model_info.vertex_data.indices.size());

    const auto& vertices = pmesh->get_vertex_buffer();
    for(int ii=0; ii<pmesh->get_nv(); ++ii)
    {
        assert(vertices[ii].position_ == model_info.vertex_data.vertices[ii].position_);
        assert(vertices[ii].normal_   == model_info.vertex_data.vertices[ii].normal_);
        assert(vertices[ii].tangent_  == model_info.vertex_data.vertices[ii].tangent_);
        assert(vertices[ii].uv_       == model_info.vertex_data.vertices[ii].uv_);
        assert(vertices[ii].weight_   == model_info.vertex_data.vertices[ii].weight_);
        assert(vertices[ii].bone_id_  == model_info.vertex_data.vertices[ii].bone_id_);
    }
    const auto& indices = pmesh->get_index_buffer();
    for(int ii=0; ii<pmesh->get_ni(); ++ii)
    {
        assert(indices[ii] == model_info.vertex_data.indices[ii]);
    }
}

int main(int argc, char const *argv[])
{
    // Init WCore config and file system
    wcore::CONFIG.init();
    wcore::dbg::LOG.register_channel("wconvert",  3);
    wcore::FILESYSTEM.open_archive("../res/pack0.zip", "pack0"_h);

    // Declare work folder for models to config
    auto rootdir = wcore::CONFIG.get_root_directory();
    auto modelswork = rootdir.parent_path() / "WCoreAssetsT/models";
    if(fs::exists(modelswork))
        wcore::CONFIG.set("root.folders.modelswork"_h, std::cref(modelswork));
    else
        DLOGF("Models work directory does not exist.", "wconvert");

    // * Load animated models and export them to my formats
    wconvert::AnimatedModelImporter am_importer;
    wconvert::StaticModelImporter sm_importer;
    wconvert::XMLSkeletonExporter skel_exporter;
    wconvert::BinaryMeshExporter mesh_exporter;

    // Convert all .dae files in work directory
    std::vector<std::string> files;
    read_directory(modelswork, files);

    for(auto& filename: files)
    {
        if(check_extension(filename, ".dae"))
        {
            DLOGS("Converting <n>" + filename + "</n>", "wconvert", Severity::LOW);
            wconvert::AnimatedModelInfo model_info;
            if(am_importer.load_model(filename, model_info))
            {
                skel_exporter.export_skeleton(model_info);
                mesh_exporter.export_mesh(model_info);

                //test_read_binary(filename, model_info);
            }
            DLOGES("wconvert", Severity::LOW);
        }
        else if(check_extension(filename, ".obj"))
        {
            DLOGS("Converting <n>" + filename + "</n>", "wconvert", Severity::LOW);
            wconvert::StaticModelInfo model_info;
            if(sm_importer.load_model(filename, model_info))
                mesh_exporter.export_mesh(model_info);
            DLOGES("wconvert", Severity::LOW);

        }
    }

    return 0;
}
