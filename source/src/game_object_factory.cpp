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
    register_component_factory("Model"_h, [&](WEntity& target, rapidxml::xml_node<>* cmp_node)
    {
        auto cmp_model = target.add_component<component::WCModel>();
        std::string model_name;
        if(xml::parse_attribute(cmp_node, "name", model_name))
        {
            cmp_model->model = model_factory_->make_model_instance(H_(model_name.c_str()));
            cmp_model->model->get_mesh().set_buffer_batch(BufferToken::Batch::INSTANCE);
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

std::shared_ptr<SurfaceMesh> GameObjectFactory::preload_mesh_entity(hash_t blueprint)
{
    // From entity blueprint, get model instance name
    rapidxml::xml_node<>* bp_node = entity_factory_->get_blueprint_node(blueprint);
    rapidxml::xml_node<>* cmp_model_node = bp_node->first_node("Components")->first_node("Model");

    if(cmp_model_node == nullptr) return nullptr;
    std::string model_name;
    if(!xml::parse_attribute(cmp_model_node, "name", model_name)) return nullptr;

    return preload_mesh_model_instance(H_(model_name.c_str()));
}

void GameObjectFactory::cache_cleanup()
{
    model_factory_->cache_cleanup();
}

static int frame_count = 0;
void GameObjectFactory::update(const GameClock& clock)
{
    // Cleanup cache every 10s or so.
    if(++frame_count > 10*60)
    {
        cache_cleanup();
        frame_count = 0;
    }
}

/*
void GameObjectFactory::init_events(InputHandler& handler)
{

}
*/


} // namespace wcore
