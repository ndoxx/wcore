#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include <functional>

#include "singleton.hpp"
#include "game_system.h"
#include "buffer_unit.hpp"
#include "vertex_array.hpp"
#include "chunk.h"
#include "wentity.h"
#include "octree.hpp"
#ifndef __DISABLE_EDITOR__
#include "editor.h"
#endif

namespace wcore
{

struct Vertex3P3N3T2U;
struct ModelRenderInfo;
class Model;
class SkyBox;
class Camera;
class Light;
class HeightMap;
class InputHandler;

struct StaticOctreeData
{
    Model* model;
    int key;

    bool operator==(const StaticOctreeData& other)
    {
        return key == other.key;
    }
};

class Scene: public GameSystem
{
private:
    typedef Octree<BoundingRegion, StaticOctreeData> StaticOctree;

    BufferUnit<Vertex3P3N3T2U>  instance_buffer_unit_;
    VertexArray<Vertex3P3N3T2U> instance_vertex_array_;

    std::map<uint32_t, Chunk*> chunks_;
    std::map<hash_t, std::weak_ptr<Model>> ref_models_;
    std::vector<uint64_t> displayable_entities_;
    StaticOctree static_octree;

    std::shared_ptr<SkyBox> skybox_;           // Optional skybox
    std::shared_ptr<Light> directional_light_; // The only directionnal light
    pCamera camera_;                           // Freefly camera (editor)
    pCamera light_camera_;                     // Virtual camera for shadow mapping
    uint32_t chunk_size_m_;                    // Size of chunks in meters
    uint32_t current_chunk_index_;             // Index of the chunk the camera is in
    math::i32vec2 current_chunk_coords_;       // Coordinates of the chunk the camera is in
    std::vector<uint32_t> chunks_order_;       // Permutation vector for chunk ordering

    static uint32_t SHADOW_WIDTH;              // Width of shadow map
    static uint32_t SHADOW_HEIGHT;             // Height of shadow map

public:
    Scene();
   ~Scene();

    float shadow_bias_;                     // Bias parameter for PCF shadow mapping

    // Static octree access
    inline StaticOctree& get_static_octree() { return static_octree; }
    void populate_static_octree(uint32_t chunk_index);

    // Getters
    inline pCamera get_camera_shared()            { return camera_; }
    inline Camera& get_camera()                   { return *camera_; }
    inline const Camera& get_camera() const       { return *camera_; }
    inline Camera& get_light_camera()             { return *light_camera_; }
    inline const Camera& get_light_camera() const { return *light_camera_; }
    inline std::weak_ptr<Light> get_directional_light_nc()           { return std::weak_ptr<Light>(directional_light_); }
    inline std::weak_ptr<const Light> get_directional_light() const  { return std::weak_ptr<const Light>(directional_light_); }

    inline Light& get_light(uint32_t index, uint32_t chunk_index)   { return *chunks_.at(chunk_index)->lights_[index]; }
    inline uint32_t get_chunk_size_meters() const                   { return chunk_size_m_; }
    inline uint32_t get_current_chunk_index() const                 { return current_chunk_index_; }
    inline uint32_t get_num_loaded_chunks() const                   { return chunks_.size(); }
    inline const math::i32vec2& get_current_chunk_coords() const    { return current_chunk_coords_; }
    inline const math::i32vec2& get_chunk_coordinates(uint32_t chunk_index) const { return chunks_.at(chunk_index)->get_coordinates(); }

    inline bool has_chunk(uint32_t chunk_index) const               { return chunks_.find(chunk_index) != chunks_.end(); }
    inline math::vec3 get_chunk_center(uint32_t chunk_index) const;
    inline const Chunk& get_chunk(uint32_t chunk_index) const       { return *chunks_.at(chunk_index); }
    void get_loaded_chunks_coords(std::vector<math::i32vec2>& coord_list) const;
    void get_far_chunks(uint32_t unload_radius, std::vector<uint32_t>& chunk_list) const;

    const HeightMap& get_heightmap(uint32_t chunk_index) const;
    float get_height(math::vec3 position) const;
    bool has_terrain(uint32_t chunk_index) const;

    inline bool has_skybox() const          { return (skybox_ != nullptr); }
    inline const SkyBox& get_skybox() const { return *skybox_; }

    inline uint32_t get_vertex_count() const;
    inline uint32_t get_triangles_count() const;

    // Setters
    inline void add_directional_light(std::shared_ptr<Light> light) { directional_light_ = light; }

    void set_chunk_size_meters(uint32_t chunk_size_m);
    void add_chunk(const math::i32vec2& coords);
    inline void remove_chunk(const math::i32vec2& coords);
    inline void remove_chunk(uint32_t chunk_index);
    inline void clear_chunks();

    void submit_mesh_instance(std::shared_ptr<SurfaceMesh> mesh);
    void load_instance_geometry();

    void add_model_instance(std::shared_ptr<Model> model, uint32_t chunk_index);
    void add_model(std::shared_ptr<Model> model, uint32_t chunk_index);
    void add_model(pLineModel model, uint32_t chunk_index)                             { chunks_.at(chunk_index)->add_model(model); }
    void add_light(std::shared_ptr<Light> light, uint32_t chunk_index);
    void add_terrain(std::shared_ptr<TerrainChunk> terrain, uint32_t chunk_index);
    std::weak_ptr<Model> get_model_by_ref(hash_t ref);
    inline void add_position_updater(PositionUpdater* updater, uint32_t chunk_index)   { chunks_.at(chunk_index)->add_position_updater(updater); }
    inline void add_rotator(ConstantRotator* rotator, uint32_t chunk_index)            { chunks_.at(chunk_index)->add_rotator(rotator); }

    inline void set_skybox(std::shared_ptr<SkyBox> skybox) { skybox_ = skybox; }

    //uint64_t add_entity(std::shared_ptr<WEntity> entity);
    inline void register_displayable_entity(uint64_t id) { displayable_entities_.push_back(id); }

    // Methods
    // Upload given chunk geometry to OpenGL
    inline void load_geometry(uint32_t chunk_index) { if(chunk_index) chunks_.at(chunk_index)->load_geometry(); }
    // Initialize event listener
    virtual void init_events(InputHandler& handler) override;
    // Update camera and models that use basic updaters
    virtual void update(const GameClock& clock) override;
#ifndef __DISABLE_EDITOR__
    virtual void generate_widget() override;
    inline Editor* locate_editor() { return locate<Editor>("Editor"_h); }
#endif
    // Sort models within each chunk according to distance to camera
    void sort_models();
    // Sort models within each chunk according to distance to light camera
    void sort_models_light();
    // Sort chunks according to distance from their centers to camera
    void sort_chunks();
    // Visit the 4 cardinal neighbors of a given chunk
    void traverse_loaded_neighbor_chunks(uint32_t chunk_index,
                                         std::function<void(Chunk*, wcore::NEIGHBOR)> visitor);
    // Visit category of models in loaded chunks in a specified order
    void traverse_models(ModelVisitor func,
                         ModelEvaluator ifFunc=wcore::DEFAULT_MODEL_EVALUATOR,
                         wcore::ORDER order=wcore::ORDER::IRRELEVANT,
                         wcore::MODEL_CATEGORY model_cat=wcore::MODEL_CATEGORY::OPAQUE) const;
    // Visit the first model that evaluates to true in evaluator predicate (front to back search)
    void visit_model_first(ModelVisitor func, ModelEvaluator ifFunc) const;
    // Visit lights in loaded chunks
    void traverse_lights(LightVisitor func,
                         LightEvaluator ifFunc=wcore::DEFAULT_LIGHT_EVALUATOR);
    // Draw models made up of lines
    void draw_line_models(std::function<void(pLineModel)> func);
    // Draw category of 3D models in loaded chunks in a specified order
    void draw_models(std::function<void(const Model&)> prepare,
                     ModelEvaluator evaluate=wcore::DEFAULT_MODEL_EVALUATOR,
                     wcore::ORDER order=wcore::ORDER::IRRELEVANT,
                     wcore::MODEL_CATEGORY model_cat=wcore::MODEL_CATEGORY::OPAQUE) const;
    // Draw terrains in loaded chunks
    void draw_terrains(std::function<void(const TerrainChunk&)> prepare,
                       ModelEvaluator evaluate=wcore::DEFAULT_MODEL_EVALUATOR) const;

private:
    // Find which models are in view frustum
    void visibility_pass();
};

inline void Scene::remove_chunk(const math::i32vec2& coords)
{
    remove_chunk(std::hash<math::i32vec2>{}(coords));
}
inline void Scene::remove_chunk(uint32_t chunk_index)
{
    static_octree.remove_group(chunk_index);
    auto it = chunks_.find(chunk_index);
    if(it!=chunks_.end())
    {
        delete it->second;
        chunks_.erase(chunk_index);
    }
}
inline void Scene::clear_chunks()
{
    for(auto chunkEntry: chunks_)
        delete chunkEntry.second;
    chunks_.clear();
}

inline math::vec3 Scene::get_chunk_center(uint32_t chunk_index) const
{
    const math::i32vec2& coords = chunks_.at(chunk_index)->get_coordinates();
    return math::vec3((coords.x()+0.5f)*chunk_size_m_,
                      0,
                      (coords.y()+0.5f)*chunk_size_m_);
}

inline uint32_t Scene::get_vertex_count() const
{
    uint32_t count = 0;
    for(auto chunk_entry: chunks_)
        count += chunk_entry.second->get_vertex_count();
    return count;
}

inline uint32_t Scene::get_triangles_count() const
{
    uint32_t count = 0;
    for(auto chunk_entry: chunks_)
        count += chunk_entry.second->get_triangles_count();
    return count;
}

}

#endif // SCENE_H
