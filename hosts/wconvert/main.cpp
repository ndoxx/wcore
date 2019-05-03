#include <iostream>
#include <filesystem>
#include <vector>

#include "config.h"
#include "file_system.h"
#include "logger.h"
#include "animated_model_importer.h"
#include "xml_skeleton_exporter.h"
#include "binary_mesh_exporter.h"

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

void test_read_binary(const std::string& filename, const wconvert::ModelInfo& model_info)
{
    std::string stripped_filename(filename.substr(0, filename.find('.')));
    // Test read binary mesh data
    std::ifstream fin("../res/models/" + stripped_filename + ".wesh", std::ios::in | std::ios::binary);

    size_t vsize, isize;

    // Read vertex data size
    fin.read(reinterpret_cast<char*>(&vsize), sizeof(vsize));
    // Read vertex data
    std::vector<VertexAnim> vertices(vsize);
    fin.read(reinterpret_cast<char*>(&vertices[0]), vsize*sizeof(VertexAnim));
    // Read index data size
    fin.read(reinterpret_cast<char*>(&isize), sizeof(isize));
    // Read index data
    std::vector<uint32_t> indices(isize);
    fin.read(reinterpret_cast<char*>(&indices[0]), isize*sizeof(uint32_t));

    fin.close();

    assert(vsize == model_info.vertex_data.vertices.size());
    assert(isize == model_info.vertex_data.indices.size());
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
    wconvert::AnimatedModelImporter importer;
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
            wconvert::ModelInfo model_info;
            if(importer.load_model(filename, model_info))
            {
                skel_exporter.export_skeleton(model_info);
                mesh_exporter.export_mesh(model_info);

                //test_read_binary(filename, model_info);
            }
            DLOGES("wconvert", Severity::LOW);
        }
    }

    return 0;
}
