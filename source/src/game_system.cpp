#include "game_system.h"
#include "logger.h"

namespace wcore
{


#ifdef __DEBUG__
GameSystem* GameSystemContainer::get_game_system_by_name(hash_t name)
{
    auto it = game_systems_map_.find(name);
    if(it == game_systems_map_.end())
    {
        DLOGE("[GameSystemContainer] Unknown game system:", "core");
        DLOGI(std::to_string(name) + " -> " + HRESOLVE(name), "core");
        return nullptr;
    }
    else
        return it->second;
}

InitializerSystem* GameSystemContainer::get_initializer_system_by_name(hash_t name)
{
    auto it = initializer_systems_map_.find(name);
    if(it == initializer_systems_map_.end())
    {
        DLOGE("[GameSystemContainer] Unknown initializer system:", "core");
        DLOGI(std::to_string(name) + " -> " + HRESOLVE(name), "core");
        return nullptr;
    }
    else
        return it->second;
}
#endif

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
    //system->init_self();
    // Subscribe system to the events it wants to respond to
    system->init_events(handler);

    game_systems_map_.insert(std::make_pair(name, system));
    game_systems_.push_back(system);
}

void GameSystemContainer::init_game_systems()
{
    for(auto&& system: game_systems_)
        system->init_self();
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

void GameSystemContainer::unload()
{
    for(auto&& system: game_systems_)
        system->on_unload();
}


} // namespace wcore
