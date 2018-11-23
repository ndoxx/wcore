#ifndef UPDATABLE_H
#define UPDATABLE_H

namespace wcore
{

class GameClock;
class Updatable
{
public:
    virtual void update(const GameClock& clock) = 0;
};

}

#endif // UPDATABLE_H
