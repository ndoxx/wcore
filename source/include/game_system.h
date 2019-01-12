#ifndef GAME_SYSTEM_H
#define GAME_SYSTEM_H

#include "listener.h"

namespace wcore
{

class GameClock;
class InputHandler;
class GameSystem: public Listener
{
public:
    virtual void update(const GameClock& clock) {}
    virtual void init_events(InputHandler& handler) {}
};

}

#endif // GAME_SYSTEM_H
