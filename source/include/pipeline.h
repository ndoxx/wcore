#ifndef PIPELINE_H
#define PIPELINE_H

#include "math3d.h"
#include "listener.h"

#ifdef __PROFILING_RENDERERS__
    #include "clock.hpp"
    #include "moving_average.h"
    #define PROFILING_MAX_SAMPLES 1000
#endif

namespace wcore
{

class GeometryRenderer;
class ShadowMapRenderer;
class LightingRenderer;
class ForwardRenderer;
class SSAORenderer;
class BloomRenderer;
class PostProcessingRenderer;
class TextRenderer;
class DebugRenderer;
class DebugOverlayRenderer;
class InputHandler;

class RenderPipeline : public Listener
{
private:
    GeometryRenderer*        geometry_renderer_;
    ShadowMapRenderer*       shadow_map_renderer_;
    LightingRenderer*        lighting_renderer_;
    ForwardRenderer*         forward_renderer_;
    SSAORenderer*            SSAO_renderer_;
    BloomRenderer*           bloom_renderer_;
    PostProcessingRenderer*  post_processing_renderer_;
    TextRenderer*            text_renderer_;
    DebugRenderer*           debug_renderer_;
    DebugOverlayRenderer*    debug_overlay_renderer_;

    bool ssao_enabled_;
    bool bloom_enabled_;
    bool forward_enabled_;

#ifdef __PROFILING_RENDERERS__
    nanoClock frame_clock_;
    nanoClock profile_clock_;
    MovingAverage geometry_dt_fifo_;
    MovingAverage SSAO_dt_fifo_;
    MovingAverage lighting_dt_fifo_;
    MovingAverage forward_dt_fifo_;
    MovingAverage bloom_dt_fifo_;
    MovingAverage pp_dt_fifo_;
    float last_render_time_;
#endif

public:
    RenderPipeline();
    ~RenderPipeline();

    void set_pp_gamma(const math::vec3& value);
    void set_pp_fog_color(const math::vec3& value);
    void set_pp_saturation(float value);
    void set_pp_fog_density(float value);

    void toggle_fog();
    void next_bb_display_mode();
    void next_light_display_mode();
    void toggle_debug_overlay();
    void toggle_wireframe();
    void toggle_debug_info();
    void debug_overlay_next();

    void onKeyboardEvent(const WData& data);
    void render();
    void dbg_show_statistics();

#ifndef __DISABLE_EDITOR__
    void generate_widget();
#endif
};

}

#endif // PIPELINE_H
