#ifndef ENTITY_SYSTEM_H
#define ENTITY_SYSTEM_H

#include "game_system.h"
#include "wentity.h"

namespace wcore
{

class EntitySystem: public GameSystem
{
public:
    EntitySystem();

    virtual void init_self() override;
    virtual void update(const GameClock& clock) override;

    uint64_t add_entity(std::shared_ptr<WEntity> entity);

    inline WEntity& get_entity(uint64_t id) { return *entities_.at(id); }

private:
    inline uint64_t get_unique_id() { return unique_id_++; }

private:
    std::map<uint64_t, std::shared_ptr<WEntity>> entities_;
    uint64_t unique_id_; // Current unique entity id

};

} // namespace wcore


#endif // ENTITY_SYSTEM_H
