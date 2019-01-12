#include "game_system.h"

namespace wcore
{

void GameSystemContainer::register_game_system(hash_t name, GameSystem* system, InputHandler& handler)
{
    // Set system parent container
    system->parent_container_ = this;
    // Subscribe system to the events it wants to respond to
    system->init_events(handler);

    game_systems_map_.insert(std::make_pair(name, system));
    game_systems_.push_back(system);
}

#ifndef __DISABLE_EDITOR__
void GameSystemContainer::generate_widgets()
{
    for(auto&& system: game_systems_)
        system->generate_widget();
}
#endif

} // namespace wcore
