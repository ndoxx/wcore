#include "game_object_factory.h"
#include "basic_components.h"
//#include "input_handler.h"

namespace wcore
{

GameObjectFactory::GameObjectFactory():
model_factory_(new ModelFactory("assets.xml")),
entity_factory_(new EntityFactory("entity.xml"))
{
    // Register component model factory function here
    // bc it uses model_factory_
    entity_factory_->register_component_factory("Model"_h, [&](WEntity& target, rapidxml::xml_node<>* cmp_node)
    {
        auto cmp_model = target.add_component<component::WCModel>();
        std::string model_name;
        if(xml::parse_attribute(cmp_node, "name", model_name))
        {
            cmp_model->model = model_factory_->make_model_instance(H_(model_name.c_str()));
            return true;
        }
        return false;
    });
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
