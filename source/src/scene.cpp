#include <cstdlib>
#include <ctime>
#include <random>
#include <algorithm>
#include <sstream>

#include "scene.h"
#include "camera.h"
#include "texture.h"
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

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;

uint32_t Scene::SHADOW_HEIGHT = 1024;
uint32_t Scene::SHADOW_WIDTH  = 1024;

Scene::Scene():
camera_(std::make_shared<Camera>(GLB.WIN_W, GLB.WIN_H)),
light_camera_(std::make_shared<Camera>(1, 1)),
chunk_size_m_(32),
current_chunk_index_(0)
{
    // Disable light camera frustum update
    light_camera_->disable_frustum_update();

    // Get shadow map size
    CONFIG.get(H_("root.render.shadowmap.width"), SHADOW_WIDTH);
    CONFIG.get(H_("root.render.shadowmap.height"), SHADOW_HEIGHT);

    // Register debug info fields
    DINFO.register_text_slot(H_("sdiPosition"), vec3(0.2,0.9,1.0));
    DINFO.register_text_slot(H_("sdiAngles"), vec3(0.2,0.9,1.0));
    DINFO.register_text_slot(H_("sdiChunk"), vec3(0.2,0.9,1.0));
}

Scene::~Scene()
{
    // Delete chunks
    for(auto&& [key, chunk]: chunks_)
        delete chunk;
}

void Scene::set_chunk_size_meters(uint32_t chunk_size_m)
{
    chunk_size_m_ = chunk_size_m;
}

void Scene::add_chunk(const math::i32vec2& coords)
{
    Chunk* chunk = new Chunk(coords);
#ifdef __DEBUG__
    auto it = chunks_.find(chunk->get_index());
    if(it!=chunks_.end())
    {
        DLOGE("[Scene] Chunk <n>" + std::to_string(chunk->get_index()) + "</n> "
            + "already loaded. Possible hash collision.", "scene", Severity::CRIT);
        delete chunk;
        return;
    }
#endif
    chunks_.insert(std::make_pair(chunk->get_index(), chunk));

    // Setup static octree if first chunk inserted
    if(!static_scene_graph_.is_initialized())
    {
        BoundingRegion bounds({float(coords.x())*chunk_size_m_, (float(coords.x())+1)*chunk_size_m_,
                               float(coords.y())*chunk_size_m_, (float(coords.y())+1)*chunk_size_m_,
                               0.f, float(chunk_size_m_)});
        static_scene_graph_.set_root_bounding_region(bounds);
    }
}

void Scene::populate_scene_graph(uint32_t chunk_index)
{
    Chunk* chunk = chunks_.at(chunk_index);
    // Populate static octree with chunk content
    chunk->traverse_models([&](pModel pmdl, uint32_t chunk_index)
    {
        StaticOctreeData data;
        data.model = pmdl;
        // Use chunk index as a group id for later removal
        static_scene_graph_.insert(StaticOctree::DataT(pmdl->get_AABB().get_bounding_region(),
                                                       data,
                                                       chunk_index));
    });
    static_scene_graph_.propagate();
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
        chunk->bind_line_vertex_array();
        chunk->traverse_line_models([&](pLineModel pmdl)
        {
            func(pmdl);
            chunk->draw_line(pmdl->get_mesh().get_n_elements(),
                             pmdl->get_mesh().get_buffer_offset());
        });
        //GFX::unbind_vertex_array();
    }
}

void Scene::draw_models(std::function<void(pModel)> prepare,
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
                OBB& obb = chunk->get_terrain_nc()->get_OBB();
                // Frustum cull entire chunk
                if(!camera_->frustum_collides(obb))
                    continue;
            }

            // Bind VAO, and draw models
            chunk->bind_vertex_array();
            chunk->traverse_models([&](pModel pmdl, uint32_t chunk_index)
            {
                prepare(pmdl);
                chunk->draw(pmdl->get_mesh().get_n_elements(),
                            pmdl->get_mesh().get_buffer_offset());
            }, evaluate, order, model_cat);
            //GFX::unbind_vertex_array();
        }
        // TERRAINS
        // Terrains are heavily occluded by the static geometry on top,
        // so we draw them last so as to maximize depth test fails
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
                OBB& obb = chunk->get_terrain_nc()->get_OBB();
                // Frustum cull entire chunk
                if(!camera_->frustum_collides(obb))
                    continue;
            }

            // Bind VAO, and draw terrains
            chunk->bind_vertex_array();
            pModel pterrain = chunk->get_terrain_nc();
            prepare(pterrain);
            chunk->draw(pterrain->get_mesh().get_n_elements(),
                        pterrain->get_mesh().get_buffer_offset());
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
            AABB& aabb = chunk->get_terrain_nc()->get_AABB();
            // Frustum cull entire chunk
            if(!camera_->frustum_collides(aabb))
                continue;

            // Bind VAO, and draw models
            chunk->bind_blend_vertex_array();
            chunk->traverse_models([&](pModel pmdl, uint32_t chunk_index)
            {
                prepare(pmdl);
                chunk->draw_transparent(pmdl->get_mesh().get_n_elements(),
                                        pmdl->get_mesh().get_buffer_offset());
            }, evaluate, order, model_cat);
            //GFX::unbind_vertex_array();
        }
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

void Scene::add_terrain(pTerrain terrain, uint32_t chunk_index)
{
    // * Add terrain to chunk
    chunks_.at(chunk_index)->terrain_ = terrain;
}

void Scene::onMouseEvent(const WData& data)
{
    const MouseData& md = static_cast<const MouseData&>(data);
    get_camera()->update_orientation(md.dx, md.dy);
}

void Scene::onKeyboardEvent(const WData& data)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case H_("k_run"):
            camera_->set_speed(Camera::SPEED_FAST);
            break;
        case H_("k_walk"):
            camera_->set_speed(Camera::SPEED_SLOW);
            break;
        case H_("k_forward"):
            camera_->move_forward();
            break;
        case H_("k_backward"):
            camera_->move_backward();
            break;
        case H_("k_strafe_left"):
            camera_->strafe_left();
            break;
        case H_("k_strafe_right"):
            camera_->strafe_right();
            break;
        case H_("k_ascend"):
            camera_->ascend();
            break;
        case H_("k_descend"):
            camera_->descend();
            break;
    }
}

void Scene::visibility_pass()
{
    traverse_models([&](std::shared_ptr<Model> pmodel, uint32_t chunk_id)
    {
        // Non cullable models are passed
        if(!pmodel->can_frustum_cull())
        {
            pmodel->set_visibility(true);
            return;
        }
/*
        // Get model AABB
        AABB& aabb = pmodel->get_AABB();
        // Frustum culling
        pmodel->set_visibility(camera_->frustum_collides(aabb));
*/
        // Get model OBB
        OBB& obb = pmodel->get_OBB();
        // Frustum culling
        pmodel->set_visibility(camera_->frustum_collides(obb));
    });
}


void Scene::update(const GameClock& clock)
{
    float scaled_dt = clock.get_scaled_frame_duration();
    float dt = clock.get_frame_duration();

    // Update camera
    camera_->update(dt);

    // Tightly fit light camera orthographic frustum to view frustum bounding box
    light_camera_->set_orthographic_tight_fit(*camera_,
                                              directional_light_->get_position(),
                                              1.0f/SHADOW_WIDTH,
                                              1.0f/SHADOW_HEIGHT);

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
        DINFO.display(H_("sdiPosition"), ss.str());

        ss.str("");
        ss << "Yaw: " << camera_->get_yaw()
           << " Pitch: " << camera_->get_pitch();
        DINFO.display(H_("sdiAngles"), ss.str());

        // Display number of loaded chunks
        ss.str("");
        ss << "Loaded chunks: " << get_num_loaded_chunks();
        DINFO.display(H_("sdiChunk"), ss.str());
    }
}

const HeightMap& Scene::get_heightmap(uint32_t chunk_index) const
{
    return chunks_.at(chunk_index)->get_terrain()->get_heightmap();
}

float Scene::get_height(math::vec3 position) const
{
    i32vec2 chunk_coords((uint32_t)floor(position.x()/chunk_size_m_),
                         (uint32_t)floor(position.y()/chunk_size_m_));
    uint32_t chunk_index = std::hash<i32vec2>{}(chunk_coords);
    if(has_chunk(chunk_index))
    {
        vec2 lcp(fmod(position.x(), chunk_size_m_),
                 fmod(position.z(), chunk_size_m_));
        return chunks_.at(chunk_index)->get_terrain()
                                      ->get_heightmap()
                                      .get_height(lcp);
    }
    return 0.0f;
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
