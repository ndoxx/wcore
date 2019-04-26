#include <cstdlib>
#include <ctime>
#include <random>
#include <algorithm>
#include <sstream>

#include "scene.h"
#include "camera.h"
#include "texture.h"
#include "sky.h"
#include "logger.h"
#include "debug_info.h"
#include "lights.h"
#include "motion.hpp"
#include "input_handler.h"
#include "terrain_patch.h"
#include "height_map.h"
#include "globals.h"
#include "config.h"
#include "game_clock.h"
#include "camera_controller.h"
#include "basic_components.h"
#include "entity_system.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;

Scene::Scene():
instance_buffer_unit_(),
skybox_(nullptr),
camera_(std::make_shared<Camera>(GLB.WIN_W, GLB.WIN_H)),
light_camera_(std::make_shared<Camera>(1, 1)),
chunk_size_m_(32),
current_chunk_index_(0)
{
    // Disable light camera frustum update and make it a "look at" camera
    light_camera_->disable_frustum_update();
    light_camera_->set_view_policy(Camera::ViewPolicy::DIRECTIONAL);
    light_camera_->set_look_at(vec3(0));

    // Register debug info fields
    DINFO.register_text_slot("sdiPosition"_h, vec3(0.2,0.9,1.0));
    DINFO.register_text_slot("sdiAngles"_h, vec3(0.2,0.9,1.0));
    DINFO.register_text_slot("sdiChunk"_h, vec3(0.2,0.9,1.0));
}

Scene::~Scene()
{
    // Delete chunks
    for(auto&& [key, chunk]: chunks_)
        delete chunk;
}

void Scene::init_events(InputHandler& handler)
{

}

void Scene::set_chunk_size_meters(uint32_t chunk_size_m)
{
    chunk_size_m_ = chunk_size_m;
}

void Scene::submit_mesh_instance(std::shared_ptr<SurfaceMesh> mesh)
{
    instance_buffer_unit_.submit(*mesh);
    mesh->set_buffer_batch(BufferToken::Batch::INSTANCE);
}

void Scene::load_instance_geometry()
{
    instance_buffer_unit_.upload();
}

void Scene::add_chunk(const math::i32vec2& coords)
{
    Chunk* chunk = new Chunk(coords);
#ifdef __DEBUG__
    auto it = chunks_.find(chunk->get_index());
    if(it!=chunks_.end())
    {
        DLOGE("[Scene] Chunk <n>" + std::to_string(chunk->get_index()) + "</n> "
            + "already loaded. Possible hash collision.", "scene");
        delete chunk;
        return;
    }
#endif
    chunks_.insert(std::make_pair(chunk->get_index(), chunk));

    // Setup static octree if first chunk inserted
    if(!static_octree.is_initialized())
    {
        BoundingRegion bounds({float(coords.x())*chunk_size_m_, (float(coords.x())+1)*chunk_size_m_-1,
                               float(coords.y())*chunk_size_m_, (float(coords.y())+1)*chunk_size_m_-1,
                               0.f, float(chunk_size_m_)-1});
        static_octree.set_root_bounding_region(bounds);
    }
}

void Scene::populate_static_octree(uint32_t chunk_index)
{
    Chunk* chunk = chunks_.at(chunk_index);
    // Populate static octree with chunk content
    chunk->traverse_models([&](Model& model, uint32_t chunk_index)
    {
        StaticOctreeData data;
        data.model = &model;
        // Use chunk index as a group id for later removal
        static_octree.insert(StaticOctree::DataT(model.get_AABB().get_bounding_region(),
                                                 data,
                                                 chunk_index));
    });

    static_octree.propagate();
}

void Scene::sort_chunks()
{
    // Initialize order lists
    chunks_order_.resize(chunks_.size());
    uint32_t ii = 0;
    for(auto&& [key, chunk]: chunks_)
        chunks_order_[ii++] = key;

    // Get camera position
    const vec3& cam_pos = camera_->get_position();

    // Sort order list according to chunk center distance
    std::sort(chunks_order_.begin(), chunks_order_.end(),
    [&](const uint32_t& a, const uint32_t& b)
    {
        float dist_a = norm2(get_chunk_center(a)-cam_pos);
        float dist_b = norm2(get_chunk_center(b)-cam_pos);
        return (dist_a < dist_b); // sort front to back
    });
}

// Sort models front to back with respect to camera position in all chunks
void Scene::sort_models()
{
    // Gen entity system
    auto* entity_system = locate<EntitySystem>("EntitySystem"_h);

    // * Sort entities
    const vec3& cam_pos = camera_->get_position();
    std::sort(displayable_entities_.begin(), displayable_entities_.end(),
    [&](const uint64_t& a, const uint64_t& b)
    {
        float dist_a = norm2(entity_system->get_entity(a).get_component<component::WCModel>()->model->get_position()-cam_pos);
        float dist_b = norm2(entity_system->get_entity(b).get_component<component::WCModel>()->model->get_position()-cam_pos);
        return (dist_a < dist_b); // sort front to back
    });

    // * Sort models in chunks
    for(auto&& [key, chunk]: chunks_)
        chunk->sort_models(camera_);
}

void Scene::sort_models_light()
{
    for(auto&& [key, chunk]: chunks_)
        chunk->sort_models(light_camera_);
}


void Scene::traverse_models(ModelVisitor func,
                            ModelEvaluator ifFunc,
                            wcore::ORDER order,
                            wcore::MODEL_CATEGORY model_cat) const
{
    //Traverse chunks front to back for opaque geometry
    if(model_cat==wcore::MODEL_CATEGORY::OPAQUE || model_cat==wcore::MODEL_CATEGORY::IRRELEVANT)
    {
        for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
        {
            Chunk* chunk = chunks_.at(chunks_order_[ii]);
            chunk->traverse_models(func, ifFunc, order, model_cat);
        }
    }
    //Traverse chunks back to front for transparent geometry
    else if(model_cat==wcore::MODEL_CATEGORY::TRANSPARENT)
    {
        for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
        {
            uint32_t index = chunks_order_.size()-ii-1;
            Chunk* chunk = chunks_.at(chunks_order_[index]);
            chunk->traverse_models(func, ifFunc, order, model_cat);
        }
    }
}

void Scene::visit_model_first(ModelVisitor func, ModelEvaluator ifFunc) const
{
    for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
    {
        Chunk* chunk = chunks_.at(chunks_order_[ii]);
        if(chunk->visit_model_first(func, ifFunc))
            break;
    }
}

void Scene::draw_line_models(std::function<void(pLineModel)> func)
{
    //Traverse chunks front to back
    for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
    {
        Chunk* chunk = chunks_.at(chunks_order_[ii]);
        //chunk->bind_line_vertex_array();
        chunk->traverse_line_models([&](pLineModel pmdl)
        {
            func(pmdl);
            /*chunk->draw_line(pmdl->get_mesh().get_n_elements(),
                             pmdl->get_mesh().get_buffer_offset());*/
            chunk->draw(pmdl->get_mesh().get_buffer_token());
        });
        //GFX::unbind_vertex_array();
    }
}

void Scene::draw_models(std::function<void(const Model&)> prepare,
                        ModelEvaluator evaluate,
                        wcore::ORDER order,
                        wcore::MODEL_CATEGORY model_cat) const
{
    //Traverse chunks front to back for opaque geometry
    if(model_cat==wcore::MODEL_CATEGORY::OPAQUE)
    {
        // STATIC MODELS
        for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
        {
            Chunk* chunk = chunks_.at(chunks_order_[ii]);
            // * Chunk frustum culling
            // Always traverse the chunk we're at, else, frustum cull
            if(chunk->get_index()!=current_chunk_index_)
            {
                // ----- /!\ (APPROX) /!\ -----
                // Is chunk visible? ~= Is terrain visible? (when viewed from the top)

                // Get terrain chunk OBB
                if(chunk->has_terrain())
                {
                    OBB& obb = chunk->get_terrain_nc().get_OBB();
                    // Frustum cull entire chunk
                    if(!camera_->frustum_collides(obb))
                        continue;
                }
            }

            // Draw models
            chunk->traverse_models([&](const Model& model, uint32_t chunk_index)
            {
                prepare(model);
                const BufferToken& token = model.get_mesh().get_buffer_token();
                if(token.batch == BufferToken::Batch::INSTANCE)
                {
                    // Instance buffers are owned by scene
                    instance_buffer_unit_.draw(token);
                }
                else
                    chunk->draw(token);
            }, evaluate, order, model_cat);
        }
        // ENTITIES WITH MODEL INSTANCES
        auto* entity_system = locate<EntitySystem>("EntitySystem"_h);
        for(uint64_t id: displayable_entities_)
        {
            const auto& entity = entity_system->get_entity(id);
            Model& e_model = *entity.get_component<component::WCModel>()->model;
            if(evaluate(e_model))
            {
                prepare(e_model);
                const BufferToken& token = e_model.get_mesh().get_buffer_token();
                if(token.batch == BufferToken::Batch::INSTANCE)
                {
                    // Instance buffers are owned by scene
                    instance_buffer_unit_.draw(token);
                }
            }
        }
    }
    //Traverse chunks back to front for transparent geometry
    else if(model_cat==wcore::MODEL_CATEGORY::TRANSPARENT)
    {
        for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
        {
            uint32_t index = chunks_order_.size()-ii-1;
            Chunk* chunk = chunks_.at(chunks_order_[index]);
            // Is chunk visible?
            // Get terrain chunk AABB (APPROX)
            if(chunk->has_terrain())
            {
                AABB& aabb = chunk->get_terrain_nc().get_AABB();
                // Frustum cull entire chunk
                if(!camera_->frustum_collides(aabb))
                    continue;
            }

            // Draw models
            chunk->traverse_models([&](const Model& model, uint32_t chunk_index)
            {
                prepare(model);
                chunk->draw(model.get_mesh().get_buffer_token());
            }, evaluate, order, model_cat);
        }
    }
}

void Scene::draw_terrains(std::function<void(const TerrainChunk&)> prepare,
                          ModelEvaluator evaluate) const
{
    for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
    {
        Chunk* chunk = chunks_.at(chunks_order_[ii]);
        if(!chunk->has_terrain()) continue;
        // * Chunk frustum culling
        // Always traverse the chunk we're at, else, frustum cull
        if(chunk->get_index()!=current_chunk_index_)
        {
            // ----- /!\ (APPROX) /!\ -----
            // Is chunk visible? ~= Is terrain visible? (when viewed from the top)

            // Get terrain chunk OBB
            OBB& obb = chunk->get_terrain_nc().get_OBB();
            // Frustum cull entire chunk
            if(!camera_->frustum_collides(obb))
                continue;
        }

        // Draw terrains
        TerrainChunk& terrain = chunk->get_terrain_nc();
        prepare(terrain);
        chunk->draw(terrain.get_mesh().get_buffer_token());
    }
}

void Scene::traverse_lights(LightVisitor func,
                            LightEvaluator ifFunc)
{
    //Traverse chunks by order
    for(uint32_t ii=0; ii<chunks_order_.size(); ++ii)
        chunks_.at(chunks_order_[ii])->traverse_lights(func, ifFunc);
}

void Scene::traverse_loaded_neighbor_chunks(uint32_t chunk_index,
                                            std::function<void(Chunk*, wcore::NEIGHBOR)> visitor)
{
    // Get coordinates of chunk
    const i32vec2& chunk_coords = get_chunk_coordinates(chunk_index);

    if(chunk_coords.x()>0)
    {
        uint32_t nei_index = std::hash<i32vec2>{}(i32vec2(chunk_coords.x()-1, chunk_coords.y()));
        if(has_chunk(nei_index))
            visitor(chunks_[nei_index], wcore::NEIGHBOR::WEST);
    }
    if(chunk_coords.x()<std::numeric_limits<uint32_t>::max())
    {
        uint32_t nei_index = std::hash<i32vec2>{}(i32vec2(chunk_coords.x()+1, chunk_coords.y()));
        if(has_chunk(nei_index))
            visitor(chunks_[nei_index], wcore::NEIGHBOR::EAST);
    }
    if(chunk_coords.y()>0)
    {
        uint32_t nei_index = std::hash<i32vec2>{}(i32vec2(chunk_coords.x(), chunk_coords.y()-1));
        if(has_chunk(nei_index))
            visitor(chunks_[nei_index], wcore::NEIGHBOR::SOUTH);
    }
    if(chunk_coords.y()<std::numeric_limits<uint32_t>::max())
    {
        uint32_t nei_index = std::hash<i32vec2>{}(i32vec2(chunk_coords.x(), chunk_coords.y()+1));
        if(has_chunk(nei_index))
            visitor(chunks_[nei_index], wcore::NEIGHBOR::NORTH);
    }
}

void Scene::add_terrain(std::shared_ptr<TerrainChunk> terrain, uint32_t chunk_index)
{
    // * Add terrain to chunk
    chunks_.at(chunk_index)->terrain_ = terrain;
}

void Scene::visibility_pass()
{
    // Entities
    auto* entity_system = locate<EntitySystem>("EntitySystem"_h);
    for(uint64_t id: displayable_entities_)
    {
        const auto& entity = entity_system->get_entity(id);
        auto e_model = entity.get_component<component::WCModel>()->model;

        // Non cullable models are passed
        if(!e_model->can_frustum_cull())
        {
            e_model->set_visibility(true);
            return;
        }
        // Get model OBB
        OBB& obb = e_model->get_OBB();
        // Frustum culling
        e_model->set_visibility(camera_->frustum_collides(obb));
    }

    // Models in chunks
    traverse_models([&](Model& model, uint32_t chunk_id)
    {
        // Non cullable models are passed
        if(!model.can_frustum_cull())
        {
            model.set_visibility(true);
            return;
        }
        // Get model OBB
        OBB& obb = model.get_OBB();
        // Frustum culling
        model.set_visibility(camera_->frustum_collides(obb));
    });
}


void Scene::update(const GameClock& clock)
{
    float scaled_dt = clock.get_scaled_frame_duration();

    // Update current chunk coordinates
    const vec3& cam_pos = camera_->get_position();
    current_chunk_coords_ = i32vec2((uint32_t)floor(cam_pos.x()/chunk_size_m_),
                                    (uint32_t)floor(cam_pos.z()/chunk_size_m_));
    current_chunk_index_ = std::hash<i32vec2>{}(current_chunk_coords_);

    // Basic chunk updaters
    for(auto&& [key, chunk]: chunks_)
    {
        chunk->update(scaled_dt);
        chunk->sort_models(camera_);
    }

    // Perform and cache OBB / frustum tests
    visibility_pass();

    // Display debug info
    if(DINFO.active())
    {
        std::stringstream ss;
        ss << "Position: " << camera_->get_position();
        DINFO.display("sdiPosition"_h, ss.str());

        ss.str("");
        ss << "Yaw: " << camera_->get_yaw()
           << " Pitch: " << camera_->get_pitch();
        DINFO.display("sdiAngles"_h, ss.str());

        // Display number of loaded chunks
        ss.str("");
        ss << "Loaded chunks: " << get_num_loaded_chunks();
        DINFO.display("sdiChunk"_h, ss.str());
    }
}

const HeightMap& Scene::get_heightmap(uint32_t chunk_index) const
{
    return chunks_.at(chunk_index)->get_terrain().get_heightmap();
}

bool Scene::has_terrain(uint32_t chunk_index) const
{
    return chunks_.at(chunk_index)->has_terrain();
}

float Scene::get_height(math::vec3 position) const
{
    i32vec2 chunk_coords((uint32_t)floor(position.x()/chunk_size_m_),
                         (uint32_t)floor(position.y()/chunk_size_m_));
    uint32_t chunk_index = std::hash<i32vec2>{}(chunk_coords);
    if(has_chunk(chunk_index))
    {
        if(!has_terrain(chunk_index)) return 0.f;

        vec2 lcp(fmod(position.x(), chunk_size_m_),
                 fmod(position.z(), chunk_size_m_));
        return chunks_.at(chunk_index)->get_terrain()
                                      .get_heightmap()
                                      .get_height(lcp);
    }
    return 0.f;
}

void Scene::get_far_chunks(uint32_t unload_radius, std::vector<uint32_t>& chunk_list) const
{
    for(auto&& [key, chunk]: chunks_)
    {
        const i32vec2& coords = chunk->get_coordinates();
        int dx = int(coords.x()) - int(current_chunk_coords_.x());
        int dy = int(coords.y()) - int(current_chunk_coords_.y());
        if(abs(dx+dy)>unload_radius)
            chunk_list.push_back(chunk->get_index());
    }
}

void Scene::get_loaded_chunks_coords(std::vector<math::i32vec2>& coord_list) const
{
    for(auto&& [key, chunk]: chunks_)
        coord_list.push_back(chunk->get_coordinates());
}

void Scene::add_model_instance(std::shared_ptr<Model> model, uint32_t chunk_index)
{
    chunks_.at(chunk_index)->add_model(model,true);
    if(model->has_reference())
    {
        ref_models_.insert(std::pair(model->get_reference(), model));
    }
}

void Scene::add_model(std::shared_ptr<Model> model, uint32_t chunk_index)
{
    chunks_.at(chunk_index)->add_model(model);
    if(model->has_reference())
        ref_models_.insert(std::pair(model->get_reference(), model));
}

std::weak_ptr<Model> Scene::get_model_by_ref(hash_t href)
{
    auto it = ref_models_.find(href);
    if(it!= ref_models_.end())
        return it->second;
    else
        return std::weak_ptr<Model>();
}

void Scene::add_light(std::shared_ptr<Light> light, uint32_t chunk_index)
{
    chunks_.at(chunk_index)->lights_.push_back(light);
    if(light->has_reference())
        ref_lights_.insert(std::pair(light->get_reference(), light));
}

std::weak_ptr<Light> Scene::get_light_by_ref(hash_t href)
{
    auto it = ref_lights_.find(href);
    if(it!= ref_lights_.end())
        return it->second;
    else
        return std::weak_ptr<Light>();
}

#ifndef __DISABLE_EDITOR__
const char* items[] = {"Freefly", "Light"};
static int current_index = 0;

void Scene::generate_widget()
{
    // PIPELINE CONTROL SECTION
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Scene"))
    {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Camera"))
        {
            ImGui::WCombo("##camsel", "Cam selection", current_index, 2, items);
            if(current_index==0)
                camera_->generate_gui_element();
            else if(current_index==1)
                light_camera_->generate_gui_element();
            ImGui::TreePop();
            ImGui::Separator();
        }
    }
}

#endif

}
