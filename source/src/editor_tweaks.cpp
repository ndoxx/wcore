#include "editor_tweaks.h"
#include "logger.h"
#include "config.h"
#include "file_system.h"

namespace wcore
{

static const char* tweaksfile = "edtweaks.xml";

EditorTweaksInitializer::~EditorTweaksInitializer()
{

}

void EditorTweaksInitializer::init_self()
{
    // Set root directory
    const fs::path& root_path_ = CONFIG.get_root_directory();
    value_map_.set_root_directory(root_path_);

    // Get stream to tweaks file
    auto pstream = FILESYSTEM.get_file_as_stream(tweaksfile, "root.folders.config"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGE("[EditorTweaksInitializer] Cannot open file: " + std::string(tweaksfile), "editor");
        return;
    }

    // Parse tweaks file
    DLOGN("[EditorTweaksInitializer] Parsing tweaks file:", "editor");
    DLOGI("<p>" + std::string(tweaksfile) + "</p>", "core");
    value_map_.parse_xml_file(*pstream);

    DLOGI("done.", "editor");
}

void EditorTweaksInitializer::serialize()
{
    DLOGN("[EditorTweaksInitializer] Saving tweaks file.", "editor");

    for(auto&& [name, serializer]: serializers_)
        serializer();

    value_map_.write_xml();

    DLOGI("done.", "editor");
}


} // namespace wcore

