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
class GameSystemContainer
{
public:
    void register_game_system(hash_t name, GameSystem* system, InputHandler& handler);
#ifndef __DISABLE_EDITOR__
    void generate_widgets();
#endif
    inline GameSystem* get_system_by_name(hash_t name) { return game_systems_map_.at(name); }
    inline std::list<GameSystem*>::iterator begin() { return game_systems_.begin(); }
    inline std::list<GameSystem*>::iterator end()   { return game_systems_.end(); }

private:
    std::map<hash_t, GameSystem*> game_systems_map_;
    std::list<GameSystem*> game_systems_;
};

class GameSystem: public Listener
{
public:
    friend class GameSystemContainer;

    virtual void update(const GameClock& clock) {}
    virtual void init_events(InputHandler& handler) {}
#ifndef __DISABLE_EDITOR__
    virtual void generate_widget() {}
#endif

protected:
    template <typename T>
    inline T* locate(hash_t system_name) { return static_cast<T*>(parent_container_->get_system_by_name(system_name)); }

private:
    GameSystemContainer* parent_container_;
};

}

#endif // GAME_SYSTEM_H
