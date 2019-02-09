#include "entity_system.h"
#include "sound_system.h"
#include "game_object_factory.h"
#include "basic_components.h"
#include "scene.h"
#include "logger.h"

namespace wcore
{

static SoundSystem* s_sound_system = nullptr;
static GameObjectFactory* s_game_object_factory = nullptr;
static Scene* s_scene = nullptr;


EntitySystem::EntitySystem():
unique_id_(0)
{

}

void EntitySystem::init_self()
{
    DLOGS("[EntitySystem] Initializing.", "entity", Severity::LOW);

    // Locate game systems
    s_sound_system        = locate<SoundSystem>("SoundSystem"_h);
    s_game_object_factory = locate<GameObjectFactory>("GameObjectFactory"_h);
    s_scene               = locate<Scene>("Scene"_h);

    DLOGN("Registering component factories.", "entity", Severity::LOW);
    // Register component factory methods in entity factory
    s_game_object_factory->register_component_factory("SoundEmitter"_h, [&](WEntity& target, rapidxml::xml_node<>* cmp_node)
    {
        auto cmp_sound = target.add_component<component::WCSoundEmitter>();
        std::string sound_name;
        if(xml::parse_node(cmp_node, "Sound", sound_name))
        {
            cmp_sound->sound_name = H_(sound_name.c_str());
            return true;
        }

        return false;
    });

    DLOGES("entity", Severity::LOW);
}

// TMP
static std::vector<uint64_t> sound_emitting_entities;
static std::map<uint64_t, int> sound_channels;

void EntitySystem::update(const GameClock& clock)
{
    // TMP
    // Use model's position vector as reference position
    for(uint64_t ent_id: sound_emitting_entities)
    {
        int channel_id = sound_channels[ent_id];
        if(channel_id == 0)
        {
            hash_t sound_name     = entities_[ent_id]->get_component<component::WCSoundEmitter>()->sound_name;
            const math::vec3& pos = entities_[ent_id]->get_component<component::WCModel>()->model->get_position();

            sound_channels[ent_id] = s_sound_system->play_sound(sound_name,
                                                                pos);
        }
        else
        {
            // update position
            const math::vec3& pos = entities_[ent_id]->get_component<component::WCModel>()->model->get_position();
            s_sound_system->set_channel_position(channel_id, pos);
        }
    }
}

uint64_t EntitySystem::add_entity(std::shared_ptr<WEntity> entity)
{
    uint64_t id = get_unique_id();
    entities_.insert(std::pair(id, entity));

    // * Subscribe entity to systems depending on signature (component composition)
    // Check for displayable components
    if(entity->has_component<component::WCModel>())
    {
        s_scene->register_displayable_entity(id);
    }
    // TMP
    if(entity->has_component<component::WCSoundEmitter>())
    {
        sound_emitting_entities.push_back(id);
        sound_channels.insert(std::pair(id,0));
    }

    return id;
}


} // namespace wcore
