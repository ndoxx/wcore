#include "game_object_factory.h"
//#include "input_handler.h"

namespace wcore
{

GameObjectFactory::GameObjectFactory():
model_factory_(new ModelFactory("assets.xml")),
entity_factory_(new EntityFactory("entity.xml"))
{

}

GameObjectFactory::~GameObjectFactory()
{
    delete entity_factory_;
    delete model_factory_;
}

/*
void GameObjectFactory::init_events(InputHandler& handler)
{

}

void GameObjectFactory::update(const GameClock& clock)
{

}
*/

} // namespace wcore
