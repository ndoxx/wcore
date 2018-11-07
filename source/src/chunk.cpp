#include "chunk.h"
#include "model.h"
#include "terrain_patch.h"
#include "material.h"
#include "camera.h"
#include "motion.hpp"

#ifdef __PROFILING_CHUNKS__
#include "clock.hpp"
#include "moving_average.h"
#define PROFILING_MAX_SAMPLES 1000

static nanoClock profile_clock_;
static MovingAverage models_traversal_fifo_(PROFILING_MAX_SAMPLES);
static MovingAverage lights_traversal_fifo_(PROFILING_MAX_SAMPLES);
static MovingAverage sorting_fifo_(PROFILING_MAX_SAMPLES);
#endif

using namespace math;

namespace wcore
{
ModelEvaluator DEFAULT_MODEL_EVALUATOR([](pModel p){return true;});
cModelEvaluator DEFAULT_CMODEL_EVALUATOR([](pcModel p){return true;});
LightEvaluator DEFAULT_LIGHT_EVALUATOR([](pLight p){return true;});
cLightEvaluator DEFAULT_CLIGHT_EVALUATOR([](pcLight p){return true;});
}

Chunk::Chunk(i32vec2 coords):
coords_(coords),
index_(std::hash<i32vec2>{}(coords)),
buffer_unit_(),
blend_buffer_unit_(),
line_buffer_unit_(GL_LINES),
vertex_array_(buffer_unit_),
blend_vertex_array_(blend_buffer_unit_),
line_vertex_array_(line_buffer_unit_)
{

}

Chunk::~Chunk()
{
    for(PositionUpdater* pu: position_updaters_)
        delete pu;
    for(ConstantRotator* cr: constant_rotators_)
        delete cr;

#ifdef __DEBUG_CHUNKS__
    DLOGN("[Chunk] Destroying chunk: <n>" + std::to_string(index_) + "</n>.");
#endif
#ifdef __PROFILING_CHUNKS__
    DLOGN("[Chunk] <n>" + std::to_string(index_) + "</n> statistics:");
    dbg_show_statistics();
#endif
}

void Chunk::add_model(pModel model)
{
    if(model->get_material().has_blend())
    {
        models_blend_.push_back(model);
        blend_models_order_.push_back(models_blend_.size()-1);
    }
    else
    {
        models_.push_back(model);
        models_order_.push_back(models_.size()-1);
    }
}

void Chunk::add_model(pLineModel model)
{
    line_models_.push_back(model);
}

// Sort models front to back with respect to camera position
void Chunk::sort_models(pCamera camera)
{
#ifdef __PROFILING_CHUNKS__
        profile_clock_.restart();
#endif

    // Get camera position
    const vec3& cam_pos = camera->get_position();

    // Sort order list according to models distance
    std::sort(models_order_.begin(), models_order_.end(),
    [&](const uint32_t& a, const uint32_t& b)
    {
        float dist_a = norm2(models_[a]->get_position()-cam_pos);
        float dist_b = norm2(models_[b]->get_position()-cam_pos);
        return (dist_a < dist_b); // sort front to back
    });
    std::sort(blend_models_order_.begin(), blend_models_order_.end(),
    [&](const uint32_t& a, const uint32_t& b)
    {
        float dist_a = norm2(models_blend_[a]->get_position()-cam_pos);
        float dist_b = norm2(models_blend_[b]->get_position()-cam_pos);
        return (dist_a > dist_b); // sort back to front
    });

#ifdef __PROFILING_CHUNKS__
        auto period = profile_clock_.get_elapsed_time();
        sorting_fifo_.push(std::chrono::duration_cast<std::chrono::duration<float>>(period).count());
#endif
}

void Chunk::traverse_models(ModelVisitor func,
                            ModelEvaluator ifFunc,
                            wcore::ORDER order,
                            wcore::MODEL_CATEGORY model_cat) const
{
#ifdef __PROFILING_CHUNKS__
        profile_clock_.restart();
#endif

    if(model_cat == wcore::MODEL_CATEGORY::OPAQUE)
    {
        if(order == wcore::ORDER::IRRELEVANT)
        {
            // Static models
            for(pModel pmodel : models_)
                if(ifFunc(pmodel))
                    func(pmodel, index_);

            // Terrain
            if(ifFunc(terrain_))
                func(terrain_, index_);
        }
        else if(order == wcore::ORDER::FRONT_TO_BACK)
        {
            // Sorted static models
            for(uint32_t ii=0; ii<models_order_.size(); ++ii)
            {
                pModel pmodel = models_[models_order_[ii]];
                if(ifFunc(pmodel))
                    func(pmodel, index_);
            }
            // Terrain
            if(ifFunc(terrain_))
                func(terrain_, index_);
        }
        else
        {
            DLOGW("[Scene] Traverse order not supported for opaque models.");
        }
    }
    else if(model_cat == wcore::MODEL_CATEGORY::TRANSPARENT)
    {
        if(order == wcore::ORDER::IRRELEVANT)
        {
            for(pModel pmodel : models_blend_)
                func(pmodel, index_);
        }
        else if(order == wcore::ORDER::BACK_TO_FRONT)
        {
            for(uint32_t ii=0; ii<blend_models_order_.size(); ++ii)
            {
                pModel pmodel = models_blend_[blend_models_order_[ii]];
                if(ifFunc(pmodel))
                    func(pmodel, index_);
            }
        }
        else
        {
            DLOGW("[Scene] Traverse order not supported for transparent models.");
        }
    }

#ifdef __PROFILING_CHUNKS__
        auto period = profile_clock_.get_elapsed_time();
        models_traversal_fifo_.push(std::chrono::duration_cast<std::chrono::duration<float>>(period).count());
#endif
}

void Chunk::traverse_line_models(std::function<void(pLineModel)> func)
{
    for(uint32_t ii=0; ii<line_models_.size(); ++ii)
    {
        pLineModel pmodel = line_models_[ii];
        func(pmodel);
    }
}


void Chunk::traverse_lights(LightVisitor func,
                            LightEvaluator ifFunc)
{
#ifdef __PROFILING_CHUNKS__
        profile_clock_.restart();
#endif

    for(pLight plight : lights_)
        if(ifFunc(plight))
            func(plight, index_);

#ifdef __PROFILING_CHUNKS__
        auto period = profile_clock_.get_elapsed_time();
        lights_traversal_fifo_.push(std::chrono::duration_cast<std::chrono::duration<float>>(period).count());
#endif
}

void Chunk::traverse_lights(cLightVisitor func,
                            cLightEvaluator ifFunc) const
{
    for(pcLight plight : lights_)
        if(ifFunc(plight))
            func(plight, index_);
}

void Chunk::load_geometry()
{
    // Submit models mesh to buffer unit then upload to OpenGL
    for(pModel pmodel: models_)
    {
#ifdef __DEBUG_MODEL_VERBOSE__
        std::stringstream ss;
        ss << "<i>Submitting</i> model: nv=" << pmodel->get_mesh().get_nv()
           << "\tni=" << pmodel->get_mesh().get_ni()
           << "\tne=" << pmodel->get_mesh().get_n_elements();
        DLOG(ss.str());
#endif //__DEBUG_MODEL_VERBOSE__
        buffer_unit_.submit(pmodel->get_mesh());
    }
    buffer_unit_.submit(terrain_->get_mesh());
    buffer_unit_.upload();

    // Geometry with alpha blending
    for(pModel pmodel: models_blend_)
    {
        blend_buffer_unit_.submit(pmodel->get_mesh());
    }
    blend_buffer_unit_.upload();

    // Line geometry
    for(pLineModel pmodel: line_models_)
    {
        line_buffer_unit_.submit(pmodel->get_mesh());
    }
    line_buffer_unit_.upload();
}

void Chunk::add_position_updater(PositionUpdater* updater)
{
    position_updaters_.push_back(updater);
}

void Chunk::add_rotator(ConstantRotator* rotator)
{
    constant_rotators_.push_back(rotator);
}

void Chunk::update(float dt)
{
    for(PositionUpdater* pu: position_updaters_)
    {
        (*pu)(dt);
    }
    for(ConstantRotator* cr: constant_rotators_)
    {
        (*cr)(dt);
    }
}

void Chunk::dbg_show_statistics()
{
#ifdef __PROFILING_CHUNKS__
    FinalStatistics models_traversal_stats = models_traversal_fifo_.get_stats();
    FinalStatistics lights_traversal_stats = lights_traversal_fifo_.get_stats();
    FinalStatistics sorting_stats          = sorting_fifo_.get_stats();

    DLOGN("Models traversal (over <z>" + std::to_string(models_traversal_fifo_.get_size()) + "</z> points): ");
    models_traversal_stats.debug_print(1e6, "µs");
    DLOGN("Lights traversal (over <z>" + std::to_string(lights_traversal_fifo_.get_size()) + "</z> points): ");
    lights_traversal_stats.debug_print(1e6, "µs");
    DLOGN("Sorting (over <z>" + std::to_string(sorting_fifo_.get_size()) + "</z> points): ");
    sorting_stats.debug_print(1e6, "µs");
#endif
}
