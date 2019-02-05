#include "editor_tweaks.h"
#include "logger.h"

namespace wcore
{

EditorTweaksInitializer::~EditorTweaksInitializer()
{

}

void EditorTweaksInitializer::init_self()
{
    DLOGN("[EditorTweaksInitializer] Parsing tweaks file.", "editor", Severity::LOW);



    DLOGI("done.", "editor", Severity::LOW);
}

void EditorTweaksInitializer::serialize()
{
    DLOGN("[EditorTweaksInitializer] Saving tweaks file.", "editor", Severity::LOW);



    DLOGI("done.", "editor", Severity::LOW);
}


} // namespace wcore

