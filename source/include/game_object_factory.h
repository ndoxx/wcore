#ifndef GAME_OBJECT_FACTORY_H
#define GAME_OBJECT_FACTORY_H

#include <memory>

#include "game_system.h"
#include "model_factory.h"
#include "entity_factory.h"

namespace wcore
{

class GameObjectFactory: public GameSystem
{
public:
    GameObjectFactory();
    ~GameObjectFactory();

    // Initialize event listener
    //virtual void init_events(InputHandler& handler) override;
    //virtual void update(const GameClock& clock) override;

    inline std::shared_ptr<Model> make_model_instance(hash_t name)
    {
        return model_factory_->make_model_instance(name);
    }
    inline std::shared_ptr<TerrainChunk> make_terrain_patch(const TerrainPatchDescriptor& desc)
    {
        return model_factory_->make_terrain_patch(desc);
    }
    inline std::shared_ptr<Model> make_model(rapidxml::xml_node<>* mesh_node,
                                             rapidxml::xml_node<>* mat_node,
                                             ModelFactory::OptRngT opt_rng)
    {
        return model_factory_->make_model(mesh_node, mat_node, opt_rng);
    }

    inline std::shared_ptr<WEntity> make_entity_blueprint(hash_t name)
    {
        return entity_factory_->make_entity_blueprint(name);
    }

    inline std::shared_ptr<SurfaceMesh> preload_mesh_instance(hash_t name)       { return model_factory_->preload_mesh_instance(name); }
    inline std::shared_ptr<SurfaceMesh> preload_mesh_model_instance(hash_t name) { return model_factory_->preload_mesh_model_instance(name); }

private:
    ModelFactory*  model_factory_;
    EntityFactory* entity_factory_;
};

} // namespace wcore

#endif // GAME_OBJECT_FACTORY_H
