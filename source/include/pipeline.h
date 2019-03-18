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

    bool onKeyboardEvent(const WData& data);
    bool onMouseEvent(const WData& data);
    void render();
    void render_gui();
    void dbg_show_statistics();

    // Enable/Disable rendering subsystems
    void set_fog_enabled(bool value);
    void set_fxaa_enabled(bool value);
    void set_shadow_mapping_enabled(bool value);
    void set_bloom_enabled(bool value);
    void set_directional_light_enabled(bool value);

    // Post processing parameter access
    void set_pp_gamma(const math::vec3& value);
    void set_pp_fog_color(const math::vec3& value);
    void set_pp_saturation(float value);
    void set_pp_fog_density(float value);
    void set_pp_exposure(float value);
    void set_pp_contrast(float value);
    void set_pp_vibrance(float value);
    void set_pp_vignette_falloff(float value);
    void set_pp_vignette_balance(float value);
    void set_pp_aberration_shift(float value);
    void set_pp_aberration_strength(float value);
    void set_pp_acc_blindness_type(int value);
    void set_pp_vibrance_balance(const math::vec3& value);

    // Lighting parameter access
    void set_shadow_bias(float value);
    void set_bright_threshold(float value);
    void set_bright_knee(float value);
    void set_shadow_slope_bias(float value);
    void set_normal_offset(float value);

#ifdef __DEBUG__
    void show_light_proxy(int mode);

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
