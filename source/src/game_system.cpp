#include "game_system.h"

namespace wcore
{

void GameSystemContainer::register_initializer_system(hash_t name, InitializerSystem* system)
{
    // Set system parent container
    system->parent_container_ = this;

    initializer_systems_map_.insert(std::make_pair(name, system));
    initializer_systems_.push_back(system);
}

void GameSystemContainer::register_game_system(hash_t name, GameSystem* system, InputHandler& handler)
{
    // Set system parent container
    system->parent_container_ = this;
    // Set internal system parameters using parent initializer systems
    system->init_self();
    // Subscribe system to the events it wants to respond to
    system->init_events(handler);

    game_systems_map_.insert(std::make_pair(name, system));
    game_systems_.push_back(system);
}

void GameSystemContainer::init()
{
    for(auto&& system: initializer_systems_)
        system->init_self();
}

void GameSystemContainer::serialize()
{
    for(auto&& system: initializer_systems_)
        system->serialize();
}

#ifndef __DISABLE_EDITOR__
void GameSystemContainer::generate_widgets()
{
    for(auto&& system: game_systems_)
        system->generate_widget();
}
#endif

} // namespace wcore
