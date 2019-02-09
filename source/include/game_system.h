#ifndef GAME_SYSTEM_H
#define GAME_SYSTEM_H

#include <map>
#include <list>
#include "wtypes.h"
#include "listener.h"

namespace wcore
{

class GameClock;
class InputHandler;
class GameSystem;
class InitializerSystem;
class GameSystemContainer
{
public:
    void register_initializer_system(hash_t name, InitializerSystem* system);
    void register_game_system(hash_t name, GameSystem* system, InputHandler& handler);
    void init();
    void serialize();
    void init_game_systems();
#ifndef __DISABLE_EDITOR__
    void generate_widgets();
#endif

#ifdef __DEBUG__
    GameSystem*        get_game_system_by_name(hash_t name);
    InitializerSystem* get_initializer_system_by_name(hash_t name);
#else
    inline GameSystem*        get_game_system_by_name(hash_t name)        { return game_systems_map_.at(name); }
    inline InitializerSystem* get_initializer_system_by_name(hash_t name) { return initializer_systems_map_.at(name); }
#endif

    inline std::list<GameSystem*>::iterator begin() { return game_systems_.begin(); }
    inline std::list<GameSystem*>::iterator end()   { return game_systems_.end(); }

private:
    std::map<hash_t, GameSystem*>        game_systems_map_;
    std::map<hash_t, InitializerSystem*> initializer_systems_map_;
    std::list<GameSystem*>        game_systems_;
    std::list<InitializerSystem*> initializer_systems_;
};

class InitializerSystem
{
public:
    friend class GameSystemContainer;

    // Initialize state of InitializerSystem
    virtual void init_self() {}
    // Write out persistent information
    virtual void serialize() {}

private:
    GameSystemContainer* parent_container_;
};

class GameSystem: public Listener
{
public:
    friend class GameSystemContainer;

    // Per-frame update
    virtual void update(const GameClock& clock) {}
    // Initialize GameSystem state
    virtual void init_self() {}
    // Initialize event handling hooks
    virtual void init_events(InputHandler& handler) {}
#ifndef __DISABLE_EDITOR__
    // Show system debug section in main debug widget
    virtual void generate_widget() {}
#endif

protected:
    // Access sibling GameSystem within parent GameSystemContainer, by name
    template <typename T>
    inline T* locate(hash_t system_name)      { return static_cast<T*>(parent_container_->get_game_system_by_name(system_name)); }
    // Access sibling InitializerSystem within parent GameSystemContainer, by name
    template <typename T>
    inline T* locate_init(hash_t system_name) { return static_cast<T*>(parent_container_->get_initializer_system_by_name(system_name)); }

private:
    GameSystemContainer* parent_container_;
};

}

#endif // GAME_SYSTEM_H
