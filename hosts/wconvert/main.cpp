#include <iostream>

#include "config.h"
#include "file_system.h"
#include "logger.h"
#include "animated_model_importer.h"
#include "xml_skeletton_exporter.h"

using namespace wcore;

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

    // Load animated models and export them to my formats
    wconvert::AnimatedModelImporter importer;
    wconvert::XMLSkelettonExporter skel_exporter;

    wconvert::ModelInfo model_info;
    if(importer.load_model("chest.dae", model_info))
    {
        /*model_info.bone_hierarchy.traverse_linear([&](const wconvert::BoneInfo& bone)
        {
            std::cout << bone.name << std::endl;
            std::cout << bone.offset_matrix << std::endl;
        });*/

        skel_exporter.export_skeletton(model_info);
    }

    return 0;
}
