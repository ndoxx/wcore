#include "pipeline.h"

#include "globals.h"
#include "gfx_driver.h"
#include "geometry_renderer.h"
#include "lighting_renderer.h"
#include "forward_renderer.h"
#include "bloom_renderer.h"
#include "SSAO_renderer.h"
#include "post_processing_renderer.h"
#include "text_renderer.h"
#include "debug_renderer.h"
#include "debug_overlay_renderer.h"
#include "shadow_map_renderer.h"

#include "l_buffer.h"
#include "g_buffer.h"
#include "SSAO_buffer.h"
#include "debug_info.h"
#include "logger.h"
#include "input_handler.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

RenderPipeline::RenderPipeline():
ssao_enabled_(false),
bloom_enabled_(true),
forward_enabled_(true)
#ifdef __PROFILING_RENDERERS__
, geometry_dt_fifo_(PROFILING_MAX_SAMPLES)
, SSAO_dt_fifo_(PROFILING_MAX_SAMPLES)
, lighting_dt_fifo_(PROFILING_MAX_SAMPLES)
, bloom_dt_fifo_(PROFILING_MAX_SAMPLES)
, pp_dt_fifo_(PROFILING_MAX_SAMPLES)
, last_render_time_(0.0f)
#endif
{
    // Buffer with facilities for Geometry pass
    GBuffer::Init(GLB.SCR_W, GLB.SCR_H);
    // Buffer with facilities for Lighting pass
    LBuffer::Init(GLB.SCR_W, GLB.SCR_H);
    // Buffer for SSAO
    SSAOBuffer::Init(GLB.SCR_W/2, GLB.SCR_H/2);

    geometry_renderer_        = new GeometryRenderer();
    shadow_map_renderer_      = new ShadowMapRenderer();
    lighting_renderer_        = new LightingRenderer(*shadow_map_renderer_);
    forward_renderer_         = new ForwardRenderer();
    SSAO_renderer_            = new SSAORenderer();
    bloom_renderer_           = new BloomRenderer();
    post_processing_renderer_ = new PostProcessingRenderer();
    text_renderer_            = new TextRenderer();
    debug_renderer_           = new DebugRenderer();
    debug_overlay_renderer_   = new DebugOverlayRenderer(*text_renderer_);

    DINFO.register_text_renderer(text_renderer_);
    text_renderer_->load_face("arial");
}

RenderPipeline::~RenderPipeline()
{
    delete debug_overlay_renderer_;
    delete debug_renderer_;
    delete text_renderer_;
    delete post_processing_renderer_;
    delete bloom_renderer_;
    delete SSAO_renderer_;
    delete forward_renderer_;
    delete lighting_renderer_;
    delete shadow_map_renderer_;
    delete geometry_renderer_;
}

void RenderPipeline::toggle_debug_info()
{
    DINFO.toggle();
}

void RenderPipeline::set_pp_gamma(const math::vec3& value)     { post_processing_renderer_->set_gamma(value); }
void RenderPipeline::set_pp_fog_color(const math::vec3& value) { post_processing_renderer_->set_fog_color(value); }
void RenderPipeline::set_pp_saturation(float value)            { post_processing_renderer_->set_saturation(value); }
void RenderPipeline::set_pp_fog_density(float value)           { post_processing_renderer_->set_fog_density(value); }
void RenderPipeline::toggle_fog()               { post_processing_renderer_->toggle_fog(); }
void RenderPipeline::next_bb_display_mode()     { debug_renderer_->next_bb_display_mode(); }
void RenderPipeline::next_light_display_mode()  { debug_renderer_->next_light_display_mode(); }
void RenderPipeline::toggle_debug_overlay()     { debug_overlay_renderer_->toggle(); }
void RenderPipeline::toggle_wireframe()         { geometry_renderer_->toggle_wireframe(); }

void RenderPipeline::debug_overlay_next()
{
    debug_overlay_renderer_->next_mode();
}

void RenderPipeline::setup_user_inputs(InputHandler& handler)
{
    handler.register_action(H_("k_tg_fog"), [&]()
    {
        toggle_fog();
    });
    handler.register_action(H_("k_bb_next_mode"), [&]()
    {
        next_bb_display_mode();
    });
    handler.register_action(H_("k_tg_line_models"), [&]()
    {
        debug_renderer_->toggle_line_models();
    });
    handler.register_action(H_("k_light_volumes_next_mode"), [&]()
    {
        next_light_display_mode();
    });
    handler.register_action(H_("k_tg_debug_overlay"), [&]()
    {
        toggle_debug_overlay();
    });
    handler.register_action(H_("k_debug_overlay_next_mode"), [&]()
    {
        debug_overlay_next();
    });
    handler.register_action(H_("k_tg_debug_info"), [&]()
    {
        toggle_debug_info();
    });
    handler.register_action(H_("k_tg_wireframe"), [&]()
    {
        toggle_wireframe();
    });
}

#ifndef __DISABLE_EDITOR__
static uint32_t frame_cnt = 0;
void RenderPipeline::generate_widget()
{
    // New window
    //ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
    ImGui::Begin("Rendering options");

    // PIPELINE CONTROL SECTION
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Pipeline control"))
    {
        ImGui::BeginChild("##pipelinectl", ImVec2(0, 3*ImGui::GetItemsLineHeightWithSpacing()));
        ImGui::Columns(2, nullptr, false);
        ImGui::Checkbox("Lighting", &lighting_renderer_->get_lighting_enabled_flag());
        ImGui::Checkbox("Shadow Mapping", &lighting_renderer_->get_shadow_enabled_flag());
        if(ImGui::Checkbox("SSAO", &ssao_enabled_))
        {
            SSAO_renderer_->set_enabled(ssao_enabled_);
            lighting_renderer_->set_SSAO_enabled(ssao_enabled_);
        }
        ImGui::NextColumn();
        if(ImGui::Checkbox("Bloom", &bloom_enabled_))
        {
            post_processing_renderer_->set_bloom_enabled(bloom_enabled_);
        }
        ImGui::Checkbox("Forward pass", &forward_enabled_);
        ImGui::EndChild();
    }

    // DEBUG DISPLAY
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Debug display"))
    {
        if(ImGui::Button("Bounding box"))
        {
            next_bb_display_mode();
        }
        ImGui::SameLine();
        switch(debug_renderer_->get_bb_display_mode())
        {
            case 0:
                ImGui::Text("Hidden");
                break;
            case 1:
                ImGui::Text("AABB");
                break;
            case 2:
                ImGui::Text("OBB");
                break;
        }

        ImGui::SameLine();
        if(ImGui::Button("Light proxy"))
        {
            next_light_display_mode();
        }
        ImGui::SameLine();
        switch(debug_renderer_->get_light_display_mode())
        {
            case 0:
                ImGui::Text("Hidden");
                break;
            case 1:
                ImGui::Text("Location");
                break;
            case 2:
                ImGui::Text("Scaled");
                break;
        }
        ImGui::SliderFloat("Wireframe", (float*)&geometry_renderer_->get_wireframe_mix_nc(), 0.0f, 1.0f);
        if(ImGui::Button("Framebuffer Peek"))
        {
            toggle_debug_overlay();
        }
        if(debug_overlay_renderer_->get_enabled_flag())
        {
            ImGui::SameLine();
            if(ImGui::Button("Next pane"))
            {
                debug_overlay_next();
            }
        }
    }

    // POST PROCESSING CONTROL
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Post-processing"))
    {
        ImGui::SliderFloat("Saturation", &post_processing_renderer_->get_saturation_nc(), 0.0f, 2.0f);
        ImGui::SliderFloat3("Gamma", (float*)&post_processing_renderer_->get_gamma_nc(), 1.0f, 2.0f);
        ImGui::SliderFloat("Exposure", (float*)&post_processing_renderer_->get_exposure_nc(), 0.1f, 5.0f);

        ImGui::Separator();
        ImGui::Text("Fog");
        ImGui::Checkbox("Enable fog", &post_processing_renderer_->get_fog_enabled_flag());
        if(post_processing_renderer_->get_fog_enabled_flag())
        {
            ImGui::SliderFloat("Density", &post_processing_renderer_->get_fog_density_nc(), 0.0f, 0.1f);
            ImGui::ColorEdit3("Color", (float*)&post_processing_renderer_->get_fog_color_nc());
        }

        ImGui::Separator();
        ImGui::Text("FXAA");
        ImGui::Checkbox("Enable FXAA", &post_processing_renderer_->get_fxaa_enabled_flag());
        if(post_processing_renderer_->get_fxaa_enabled_flag())
        {

        }
    }

#ifdef __PROFILING_RENDERERS__
    // RENDERER STATISTICS
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Render time"))
    {
        ImGui::PlotVar("Draw time", 1e3*last_render_time_, 0.0f, 16.66f);
        ImGui::PlotVar("Geometry pass", 1e3*geometry_dt_fifo_.last_element(), 0.0f, 16.66f);
        if(ssao_enabled_)
            ImGui::PlotVar("SSAO", 1e3*SSAO_dt_fifo_.last_element(), 0.0f, 16.66f);
        ImGui::PlotVar("Lighting pass", 1e3*lighting_dt_fifo_.last_element(), 0.0f, 16.66f);
        ImGui::PlotVar("Forward pass", 1e3*forward_dt_fifo_.last_element(), 0.0f, 16.66f);
        if(bloom_enabled_)
            ImGui::PlotVar("Bloom pass", 1e3*bloom_dt_fifo_.last_element(), 0.0f, 16.66f);
        ImGui::PlotVar("Post processing", 1e3*pp_dt_fifo_.last_element(), 0.0f, 16.66f);

        if(++frame_cnt>200)
        {
            frame_cnt = 0;
            ImGui::PlotVarFlushOldEntries();
        }
    }
#endif

    ImGui::End();
}
#endif //__DISABLE_EDITOR__

void RenderPipeline::render()
{
// ------- DEFERRED PASS (no blending allowed) --------------------------------
// ------- GEOMETRY PASS (draw on texture "gbuffer") --------------------------
#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        profile_clock_.restart();
        frame_clock_.restart();
#endif

        geometry_renderer_->render();

#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        auto period = profile_clock_.get_elapsed_time();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        geometry_dt_fifo_.push(dt);
#endif

// ------- SSAO (draw on texture "SSAObuffer") --------------------------------
#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        profile_clock_.restart();
#endif

        SSAO_renderer_->render();

#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        SSAO_dt_fifo_.push(dt);
#endif

// ------- LIGHTING PASS (draw on texture "screen") ---------------------------
#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        profile_clock_.restart();
#endif

        lighting_renderer_->render();

#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        lighting_dt_fifo_.push(dt);
#endif

// ------- FORWARD PASS (draw anything incompatible with deferred pass) -------
#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        profile_clock_.restart();
#endif

        if(forward_enabled_)
            forward_renderer_->render();

#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        forward_dt_fifo_.push(dt);
#endif

// ------- BLUR PASS (draw on textures "bloom_i" with i in [0,3]) -------------
#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        profile_clock_.restart();
#endif
        if(bloom_enabled_)
            bloom_renderer_->render();

#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        bloom_dt_fifo_.push(dt);
#endif

// ------- HDR, POST-PROCESSING (draw texture "screen" on actual screen) ------
#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        profile_clock_.restart();
#endif

        post_processing_renderer_->render();

#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        pp_dt_fifo_.push(dt);
#endif

// ------- OVERLAY AND TEXT (draw to screen with alpha blending) --------------
        debug_renderer_->render();
        debug_overlay_renderer_->render();
        text_renderer_->render();

#ifdef __PROFILING_RENDERERS__
        GFX::finish();
        period = frame_clock_.get_elapsed_time();
        last_render_time_ = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
#endif
}

void RenderPipeline::dbg_show_statistics()
{
#ifdef __PROFILING_RENDERERS__
    FinalStatistics geom_stats = geometry_dt_fifo_.get_stats();
    FinalStatistics SSAO_stats = SSAO_dt_fifo_.get_stats();
    FinalStatistics lighting_stats = lighting_dt_fifo_.get_stats();
    FinalStatistics forward_stats = forward_dt_fifo_.get_stats();
    FinalStatistics bloom_stats = bloom_dt_fifo_.get_stats();
    FinalStatistics pp_stats = pp_dt_fifo_.get_stats();
    uint32_t n_iter = geometry_dt_fifo_.get_size();

    DLOGN("Geometry pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "core", Severity::LOW);
    geom_stats.debug_print(1e6, "µs");
    DLOGN("SSAO pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "core", Severity::LOW);
    SSAO_stats.debug_print(1e6, "µs");
    DLOGN("Lighting pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "core", Severity::LOW);
    lighting_stats.debug_print(1e6, "µs");
    DLOGN("Forward pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "core", Severity::LOW);
    forward_stats.debug_print(1e6, "µs");
    DLOGN("Bloom pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "core", Severity::LOW);
    bloom_stats.debug_print(1e6, "µs");
    DLOGN("Post-processing pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "core", Severity::LOW);
    pp_stats.debug_print(1e6, "µs");
#endif
}

}
