#include "editor_tweaks.h"
#include "logger.h"
#include "config.h"

namespace wcore
{

EditorTweaksInitializer::~EditorTweaksInitializer()
{

}

void EditorTweaksInitializer::init_self()
{

    const fs::path& root_path_ = CONFIG.get_root_directory();
    const fs::path& conf_path_ = CONFIG.get_config_directory();
    fs::path filepath = conf_path_ / "edtweaks.xml";
    if(!fs::exists(filepath))
    {
        DLOGE("[EditorTweaksInitializer] Cannot open file: " + filepath.string(), "editor", Severity::CRIT);
        return;
    }

    DLOGN("[EditorTweaksInitializer] Parsing tweaks file:", "editor", Severity::LOW);
    DLOGI(filepath.string(), "core", Severity::LOW);

    value_map_.set_root_directory(root_path_);
    value_map_.parse_xml_file(filepath);

    DLOGI("done.", "editor", Severity::LOW);
}

void EditorTweaksInitializer::serialize()
{
    DLOGN("[EditorTweaksInitializer] Saving tweaks file.", "editor", Severity::LOW);

    for(auto&& [name, serializer]: serializers_)
        serializer();

    value_map_.write_xml();

    DLOGI("done.", "editor", Severity::LOW);
}


} // namespace wcore

