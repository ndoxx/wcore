#include "entity_system.h"
#include "sound_system.h"
#include "game_object_factory.h"
#include "logger.h"

namespace wcore
{

void EntitySystem::init_self()
{
    DLOGS("[EntitySystem] Initializing.", "entity", Severity::LOW);

    auto sound_system        = locate<SoundSystem>("SoundSystem"_h);
    auto game_object_factory = locate<GameObjectFactory>("GameObjectFactory"_h);

    DLOGN("Registering component factories.", "entity", Severity::LOW);
    // Register component factory methods in entity factory
    game_object_factory->register_component_factory("SoundEmitter"_h, [&](WEntity& target, rapidxml::xml_node<>* cmp_node)
    {

        return false;
    });

    DLOGES("entity", Severity::LOW);
}

} // namespace wcore
