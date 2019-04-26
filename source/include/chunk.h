#ifndef CHUNK_H
#define CHUNK_H

#include <vector>
#include <memory>
#include <functional>

#include "w_symbols.h"
#include "buffer_unit.hpp"
#include "math3d.h"
#include "vertex_format.h"

namespace wcore
{

class TerrainChunk;
class Model;
class LineModel;
class Camera;
class Light;
struct Vertex3P3N3T2U;
struct Vertex3P;

typedef std::shared_ptr<LineModel>    pLineModel;
typedef std::shared_ptr<Camera>       pCamera;
typedef std::weak_ptr<Light>          wpLight;
typedef std::shared_ptr<const Camera> pcCamera;
typedef std::weak_ptr<const Light>    wpcLight;

typedef std::function<void(std::shared_ptr<Model>, uint32_t)> ModelVisitorShared;
typedef std::function<void(Model&, uint32_t)> ModelVisitor;
typedef std::function<void(const Model&, uint32_t)> cModelVisitor;
typedef std::function<bool(Model&)>   ModelEvaluator;
typedef std::function<bool(const Model&)> cModelEvaluator;

typedef std::function<void(Light&, uint32_t)> LightVisitor;
typedef std::function<void(const Light&, uint32_t)> cLightVisitor;
typedef std::function<bool(Light&)> LightEvaluator;
typedef std::function<bool(const Light&)> cLightEvaluator;

extern ModelEvaluator DEFAULT_MODEL_EVALUATOR;
extern cModelEvaluator DEFAULT_CMODEL_EVALUATOR;
extern LightEvaluator DEFAULT_LIGHT_EVALUATOR;
extern cLightEvaluator DEFAULT_CLIGHT_EVALUATOR;

class PositionUpdater;
class ConstantRotator;
namespace timeEvolution
{
    class Alternating;
}

class Chunk
{
private:
    friend class Scene;

    typedef std::shared_ptr<Model> pModel;
    typedef std::shared_ptr<Light> pLight;
    typedef std::shared_ptr<TerrainChunk> pTerrain;
    typedef std::shared_ptr<const Model> pcModel;
    typedef std::shared_ptr<const Light> pcLight;
    typedef std::shared_ptr<const TerrainChunk> pcTerrain;

    math::i32vec2 coords_;
    uint32_t index_;
    BufferUnit<Vertex3P3N3T2U>  buffer_unit_;
    BufferUnit<Vertex3P3N3T2U>  terrain_buffer_unit_;
    BufferUnit<Vertex3P3N3T2U>  blend_buffer_unit_;
    BufferUnit<Vertex3P>        line_buffer_unit_;
    pTerrain terrain_;

    std::vector<pModel> models_;
    std::vector<pModel> models_blend_;
    std::vector<pModel> model_instances_;
    std::vector<pLineModel> line_models_;
    std::vector<pLight> lights_;
    std::vector<uint32_t> models_order_;
    std::vector<uint32_t> blend_models_order_;
    std::vector<uint32_t> model_instances_order_;
    std::vector<PositionUpdater*> position_updaters_;
    std::vector<ConstantRotator*> constant_rotators_;

public:
    Chunk(math::i32vec2 coords);
    ~Chunk();

    inline uint32_t get_index() const { return index_; }
    inline const math::i32vec2& get_coordinates() const { return coords_; }

    void add_model(pModel model, bool is_instance=false);
    void add_model(pLineModel model);
    void add_position_updater(PositionUpdater* updater);
    void add_rotator(ConstantRotator* rotator);

    inline void add_terrain(pTerrain terrain) { terrain_ = terrain; }
    inline void add_light(pLight light)       { lights_.push_back(light); }
    inline pLight get_light(uint32_t index)   { return lights_[index]; }
    inline const TerrainChunk& get_terrain() const  { return *terrain_; }
    inline TerrainChunk& get_terrain_nc()           { return *terrain_; }
    inline bool has_terrain() const { return (terrain_!=nullptr); }

    void sort_models(pCamera camera);

    void traverse_models(ModelVisitor func,
                         ModelEvaluator ifFunc=DEFAULT_MODEL_EVALUATOR,
                         ORDER order=ORDER::IRRELEVANT,
                         MODEL_CATEGORY model_cat=MODEL_CATEGORY::OPAQUE) const;

    bool visit_model_first(ModelVisitor func, ModelEvaluator ifFunc) const;

    void traverse_line_models(std::function<void(pLineModel)> func);

    void traverse_lights(LightVisitor func,
                         LightEvaluator ifFunc=DEFAULT_LIGHT_EVALUATOR);
    void traverse_lights(cLightVisitor func,
                         cLightEvaluator ifFunc=DEFAULT_CLIGHT_EVALUATOR) const;

    void load_geometry();

    void draw(const BufferToken& buffer_token) const;
    void update(float dt);

    void dbg_show_statistics();

    inline uint32_t get_vertex_count() const    { return buffer_unit_.get_n_vertices(); }
    inline uint32_t get_triangles_count() const { return buffer_unit_.get_n_indices()/3; }
};

}

#endif // CHUNK_H
