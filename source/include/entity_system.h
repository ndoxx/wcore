#ifndef ENTITY_SYSTEM_H
#define ENTITY_SYSTEM_H

#include "game_system.h"

namespace wcore
{

class EntitySystem: public GameSystem
{
public:
    virtual void init_self() override;
private:
};

} // namespace wcore


#endif // ENTITY_SYSTEM_H
