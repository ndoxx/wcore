#ifndef CHUNK_H
#define CHUNK_H

#include <vector>
#include <memory>
#include <functional>

#include "w_symbols.h"
#include "buffer_unit.hpp"
#include "vertex_array.hpp"
#include "math3d.h"
#include "vertex_format.h"

class TerrainChunk;
class Model;
class LineModel;
class Camera;
class Light;
struct Vertex3P3N3T2U;
struct Vertex3P;

typedef std::shared_ptr<Model>        pModel;
typedef std::shared_ptr<LineModel>    pLineModel;
typedef std::shared_ptr<TerrainChunk> pTerrain;
typedef std::shared_ptr<Camera>       pCamera;
typedef std::shared_ptr<Light>        pLight;
typedef std::weak_ptr<Light>          wpLight;
typedef std::shared_ptr<const Model>  pcModel;
typedef std::shared_ptr<const TerrainChunk> pcTerrain;
typedef std::shared_ptr<const Camera> pcCamera;
typedef std::shared_ptr<const Light>  pcLight;
typedef std::weak_ptr<const Light>    wpcLight;

typedef std::function<void(pModel, uint32_t)>   ModelVisitor;
typedef std::function<void(pcModel, uint32_t)>  cModelVisitor;
//typedef std::function<void(pModel)>   ModelVisitor;
//typedef std::function<void(pcModel)>  cModelVisitor;
typedef std::function<bool(pModel)>   ModelEvaluator;
typedef std::function<bool(pcModel)>  cModelEvaluator;

typedef std::function<void(pLight, uint32_t)>   LightVisitor;
typedef std::function<void(pcLight, uint32_t)>  cLightVisitor;
//typedef std::function<void(pLight)>   LightVisitor;
//typedef std::function<void(pcLight)>  cLightVisitor;
typedef std::function<bool(pLight)>   LightEvaluator;
typedef std::function<bool(pcLight)>  cLightEvaluator;

namespace wcore
{
extern ModelEvaluator DEFAULT_MODEL_EVALUATOR;
extern cModelEvaluator DEFAULT_CMODEL_EVALUATOR;
extern LightEvaluator DEFAULT_LIGHT_EVALUATOR;
extern cLightEvaluator DEFAULT_CLIGHT_EVALUATOR;
}

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

    math::i32vec2 coords_;
    uint32_t index_;
    BufferUnit<Vertex3P3N3T2U>  buffer_unit_;
    BufferUnit<Vertex3P3N3T2U>  blend_buffer_unit_;
    BufferUnit<Vertex3P>        line_buffer_unit_;
    VertexArray<Vertex3P3N3T2U> vertex_array_;
    VertexArray<Vertex3P3N3T2U> blend_vertex_array_;
    VertexArray<Vertex3P>       line_vertex_array_;
    pTerrain terrain_;

    std::vector<pModel> models_;
    std::vector<pModel> models_blend_;
    std::vector<pLineModel> line_models_;
    std::vector<pLight> lights_;
    std::vector<uint32_t> models_order_;
    std::vector<uint32_t> blend_models_order_;
    std::vector<PositionUpdater*> position_updaters_;
    std::vector<ConstantRotator*> constant_rotators_;

public:
    Chunk(math::i32vec2 coords);
    ~Chunk();

    inline uint32_t get_index() const { return index_; }
    inline const math::i32vec2& get_coordinates() const { return coords_; }

    void add_model(pModel model);
    void add_model(pLineModel model);
    void add_position_updater(PositionUpdater* updater);
    void add_rotator(ConstantRotator* rotator);

    inline void add_terrain(pTerrain terrain) { terrain_ = terrain; }
    inline void add_light(pLight light)       { lights_.push_back(light); }
    inline pLight get_light(uint32_t index)   { return lights_[index]; }
    inline pcTerrain get_terrain() const      { return terrain_; }
    inline pTerrain get_terrain_nc()          { return terrain_; }

    void sort_models(pCamera camera);

    void traverse_models(ModelVisitor func,
                         ModelEvaluator ifFunc=wcore::DEFAULT_MODEL_EVALUATOR,
                         wcore::ORDER order=wcore::ORDER::IRRELEVANT,
                         wcore::MODEL_CATEGORY model_cat=wcore::MODEL_CATEGORY::OPAQUE) const;

    void traverse_line_models(std::function<void(pLineModel)> func);

    void traverse_lights(LightVisitor func,
                         LightEvaluator ifFunc=wcore::DEFAULT_LIGHT_EVALUATOR);
    void traverse_lights(cLightVisitor func,
                         cLightEvaluator ifFunc=wcore::DEFAULT_CLIGHT_EVALUATOR) const;

    void load_geometry();
    inline const BufferUnit<Vertex3P3N3T2U>& get_buffer_unit() const       { return buffer_unit_; }
    inline const BufferUnit<Vertex3P3N3T2U>& get_blend_buffer_unit() const { return blend_buffer_unit_; }
    inline void bind_vertex_array()   { vertex_array_.bind(); }
    inline void bind_blend_vertex_array()   { blend_vertex_array_.bind(); }
    inline void bind_line_vertex_array()   { line_vertex_array_.bind(); }
    inline void draw(uint32_t nelements, uint32_t buffer_offset) const { buffer_unit_.draw(nelements, buffer_offset); }
    inline void draw_transparent(uint32_t nelements, uint32_t buffer_offset) const { blend_buffer_unit_.draw(nelements, buffer_offset); }
    inline void draw_line(uint32_t nelements, uint32_t buffer_offset) const { line_buffer_unit_.draw(nelements, buffer_offset); }
    void update(float dt);

    void dbg_show_statistics();

    inline uint32_t get_vertex_count() const    { return buffer_unit_.get_n_vertices(); }
    inline uint32_t get_triangles_count() const { return buffer_unit_.get_n_indices()/3; }
};

#endif // CHUNK_H
