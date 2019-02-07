#ifndef PIPELINE_H
#define PIPELINE_H

#include "math3d.h"
#include "game_system.h"

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
class GuiRenderer;
class InputHandler;

class RenderPipeline : public GameSystem
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
    GuiRenderer*             gui_renderer_;

    bool bloom_enabled_;

public:

    RenderPipeline();
    ~RenderPipeline();

    // Initialize event listener
    virtual void init_events(InputHandler& handler) override;
    virtual void init_self() override;
#ifndef __DISABLE_EDITOR__
    virtual void generate_widget() override;
#endif

    void set_pp_gamma(const math::vec3& value);
    void set_pp_fog_color(const math::vec3& value);
    void set_pp_saturation(float value);
    void set_pp_fog_density(float value);

    bool onKeyboardEvent(const WData& data);
    bool onMouseEvent(const WData& data);
    void render();
    void render_gui();
    void dbg_show_statistics();

#ifdef __DEBUG__
    void debug_draw_segment(const math::vec3& world_start,
                            const math::vec3& world_end,
                            int ttl = 60,
                            const math::vec3& color = math::vec3(0,1,0));
    void debug_draw_sphere(const math::vec3& world_pos,
                           float radius,
                           int ttl = 60,
                           const math::vec3& color = math::vec3(0,1,0));
    void debug_draw_cross3(const math::vec3& world_pos,
                           float radius,
                           int ttl = 60,
                           const math::vec3& color = math::vec3(0,1,0));
#endif

private:
#ifdef __DEBUG__
    void perform_test(); // Anything in here is bound to "k_test_key"
#endif
};

}

#endif // PIPELINE_H
