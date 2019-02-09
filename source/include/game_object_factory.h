#ifndef GAME_OBJECT_FACTORY_H
#define GAME_OBJECT_FACTORY_H

#include <memory>

#include "game_system.h"
#include "model_factory.h"
#include "entity_factory.h"

#ifdef __DEBUG__
#include "logger.h"
#endif

namespace wcore
{

class GameObjectFactory: public GameSystem
{
public:
    GameObjectFactory();
    virtual ~GameObjectFactory();

    // Initialize event listener
    //virtual void init_events(InputHandler& handler) override;
    //virtual void update(const GameClock& clock) override;

    // Create a model from instance name
    inline std::shared_ptr<Model> make_model_instance(hash_t name)
    {
        return model_factory_->make_model_instance(name);
    }
    // Create a terrain patch from descriptor
    inline std::shared_ptr<TerrainChunk> make_terrain_patch(const TerrainPatchDescriptor& desc)
    {
        return model_factory_->make_terrain_patch(desc);
    }
    // Create a model from XML nodes and an optional random engine
    // If underlying mesh is cached, mesh_is_instance will be set to true
    inline std::shared_ptr<Model> make_model(rapidxml::xml_node<>* mesh_node,
                                             rapidxml::xml_node<>* mat_node,
                                             bool& mesh_is_instance,
                                             ModelFactory::OptRngT opt_rng)
    {
        return model_factory_->make_model(mesh_node, mat_node, mesh_is_instance, opt_rng);
    }
    // Register a component creation function for the entity factory to use

    inline void register_component_factory(hash_t name, EntityFactory::ComponentCreatorFunc func)
    {
#ifdef __DEBUG__
        DLOG("New <g>component factory</g> for: ", "entity", Severity::LOW);
        DLOGI(std::to_string(name) + " -> <n>" + HRESOLVE(name) + "</n>", "entity", Severity::LOW);
#endif
        entity_factory_->register_component_factory(name, func);
    }

    // Create an entity from blueprint name
    inline std::shared_ptr<WEntity> make_entity_blueprint(hash_t name)
    {
        return entity_factory_->make_entity_blueprint(name);
    }
    // Create and cache a mesh from mesh instance name
    inline std::shared_ptr<SurfaceMesh> preload_mesh_instance(hash_t name)       { return model_factory_->preload_mesh_instance(name); }
    // Create and cache a mesh from model instance name
    inline std::shared_ptr<SurfaceMesh> preload_mesh_model_instance(hash_t name) { return model_factory_->preload_mesh_model_instance(name); }
    // Create and cache a mesh from entity blueprint
    std::shared_ptr<SurfaceMesh> preload_mesh_entity(hash_t blueprint);

private:
    ModelFactory*  model_factory_;
    EntityFactory* entity_factory_;
};

} // namespace wcore

#endif // GAME_OBJECT_FACTORY_H
