#ifndef PIPELINE_H
#define PIPELINE_H

#include "math3d.h"
#include "listener.h"

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

    bool bloom_enabled_;

public:

    RenderPipeline();
    ~RenderPipeline();

    void set_pp_gamma(const math::vec3& value);
    void set_pp_fog_color(const math::vec3& value);
    void set_pp_saturation(float value);
    void set_pp_fog_density(float value);

    void onKeyboardEvent(const WData& data);
    void render();
    void dbg_show_statistics();

#ifndef __DISABLE_EDITOR__
    void generate_widget();
#endif
};

}

#endif // PIPELINE_H
