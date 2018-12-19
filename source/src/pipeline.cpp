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
#include "gui_renderer.h"

#include "l_buffer.h"
#include "g_buffer.h"
#include "SSAO_buffer.h"
#include "debug_info.h"
#include "logger.h"
#include "input_handler.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
    #include "scene.h"
#endif

#ifdef __PROFILE__
    #include "clock.hpp"
    #include "moving_average.h"
    #define PROFILING_MAX_SAMPLES 1000
#endif

namespace wcore
{

RenderPipeline::RenderPipeline():
bloom_enabled_(true)
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
    gui_renderer_             = new GuiRenderer();

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

void RenderPipeline::set_pp_gamma(const math::vec3& value)     { post_processing_renderer_->set_gamma(value); }
void RenderPipeline::set_pp_fog_color(const math::vec3& value) { post_processing_renderer_->set_fog_color(value); }
void RenderPipeline::set_pp_saturation(float value)            { post_processing_renderer_->set_saturation(value); }
void RenderPipeline::set_pp_fog_density(float value)           { post_processing_renderer_->set_fog_density(value); }

void RenderPipeline::onKeyboardEvent(const WData& data)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case H_("k_tg_editor"):
            gui_renderer_->toggle_cursor();
            break;
        case H_("k_tg_fog"):
    		post_processing_renderer_->toggle_fog();
    		break;
        case H_("k_bb_next_mode"):
    		debug_renderer_->next_bb_display_mode();
    		break;
        case H_("k_tg_line_models"):
    		debug_renderer_->toggle_line_models();
    		break;
        case H_("k_light_volumes_next_mode"):
    		debug_renderer_->next_light_display_mode();
    		break;
        case H_("k_tg_debug_overlay"):
    		debug_overlay_renderer_->toggle();
    		break;
        case H_("k_debug_overlay_next_mode"):
    		debug_overlay_renderer_->next_mode();
    		break;
        case H_("k_tg_debug_info"):
    		DINFO.toggle();
    		break;
        case H_("k_tg_wireframe"):
    		geometry_renderer_->toggle_wireframe();
    		break;
    }
}

void RenderPipeline::onMouseEvent(const WData& data)
{
    const MouseData& md = static_cast<const MouseData&>(data);
    gui_renderer_->set_cursor_position(md.dx, md.dy);
}

#ifdef __PROFILE__
static bool profile_renderers = false;
static uint32_t frame_cnt = 0;
static float last_render_time_ = 0.0f;
static nanoClock frame_clock_;
static nanoClock profile_clock_;
static MovingAverage geometry_dt_fifo_(PROFILING_MAX_SAMPLES);
static MovingAverage SSAO_dt_fifo_(PROFILING_MAX_SAMPLES);
static MovingAverage lighting_dt_fifo_(PROFILING_MAX_SAMPLES);
static MovingAverage forward_dt_fifo_(PROFILING_MAX_SAMPLES);
static MovingAverage bloom_dt_fifo_(PROFILING_MAX_SAMPLES);
static MovingAverage pp_dt_fifo_(PROFILING_MAX_SAMPLES);
#endif

#ifndef __DISABLE_EDITOR__
const char* bb_mode_items[]       = {"Hidden", "AABB", "OBB"};
const char* light_mode_items[]    = {"Hidden", "Location", "Scaled"};
const char* acc_dalt_mode_items[] = {"Off", "Simulate", "Apply correction"};
const char* acc_blindness_items[] = {"Protanopia", "Deuteranopia", "Tritanopia"};

static int SSAO_kernel_half_size = 3;
static float SSAO_sigma = 1.8f;
static int bloom_kernel_half_size = 3;
static float bloom_sigma = 1.8f;
static bool framebuffer_peek = false;

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
        if(ImGui::Checkbox("SSAO", &SSAO_renderer_->get_active()))
        {
            lighting_renderer_->set_SSAO_enabled(SSAO_renderer_->is_active());
        }
        ImGui::NextColumn();
        if(ImGui::Checkbox("Bloom", &bloom_enabled_))
        {
            post_processing_renderer_->set_bloom_enabled(bloom_enabled_);
        }
        ImGui::Checkbox("Forward pass", &forward_renderer_->active_);
        ImGui::EndChild();
    }

    // SSAO OPTIONS
    if(SSAO_renderer_->is_active())
    {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("SSAO control"))
        {
            ImGui::SliderFloat("Radius",      &SSAO_renderer_->SSAO_radius_, 0.01f, 1.0f);
            ImGui::SliderFloat("Scalar bias", &SSAO_renderer_->SSAO_bias_, 0.0f, 1.0f);
            ImGui::SliderFloat("Vector bias", &SSAO_renderer_->SSAO_vbias_, 0.0f, 0.5f);
            ImGui::SliderFloat("Intensity",   &SSAO_renderer_->SSAO_intensity_, 0.0f, 5.0f);
            ImGui::SliderFloat("Scale",       &SSAO_renderer_->SSAO_scale_, 0.01f, 1.0f);

            ImGui::Separator();
            int ker_size = 2*SSAO_kernel_half_size-1;
            ImGui::Text("Blur: Gaussian kernel %dx%d", ker_size, ker_size);
            bool update_kernel = ImGui::SliderInt("Half-size", &SSAO_kernel_half_size, 3, 8);
            update_kernel     |= ImGui::SliderFloat("Sigma",   &SSAO_sigma, 0.5f, 2.0f);
            if(update_kernel)
            {
                SSAO_renderer_->blur_policy_.kernel_.update_kernel(2*SSAO_kernel_half_size-1, SSAO_sigma);
            }

            ImGui::SliderInt("Blur passes",   &SSAO_renderer_->blur_policy_.n_pass_, 0, 5);
            ImGui::SliderFloat("Compression", &SSAO_renderer_->blur_policy_.gamma_r_, 0.5f, 2.0f);
        }
    }

    // BLOOM OPTIONS
    if(bloom_enabled_)
    {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("Bloom control"))
        {
            ImGui::SliderFloat("Threshold", &lighting_renderer_->bright_threshold_, 0.5f, 2.0f);
            ImGui::SliderFloat("Knee", &lighting_renderer_->bright_knee_, 0.01f, 1.0f);
            int ker_size = 2*bloom_kernel_half_size-1;
            ImGui::Text("Blur: Gaussian kernel %dx%d", ker_size, ker_size);
            bool update_kernel = ImGui::SliderInt("Half-size ", &bloom_kernel_half_size, 3, 8);
            update_kernel     |= ImGui::SliderFloat("Sigma ",   &bloom_sigma, 0.5f, 3.0f);
            if(update_kernel)
            {
                bloom_renderer_->update_blur_kernel(2*bloom_kernel_half_size-1, bloom_sigma);
            }
        }
    }

    // SHADOW OPTIONS
    if(lighting_renderer_->get_shadow_enabled_flag())
    {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("Shadow control"))
        {
            ImGui::SliderFloat("Depth bias", &SCENE.shadow_bias_, 0.0f, 5.0f);
            ImGui::SliderFloat("Slope bias", &lighting_renderer_->shadow_slope_bias_, 0.0f, 0.5f);
            ImGui::SliderFloat("Normal offset", &lighting_renderer_->normal_offset_, -1.0f, 1.0f);
        }
    }

    // DEBUG
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Debug"))
    {
        ImGui::Text("Primitives");
        ImGui::Checkbox("Depth test", &debug_renderer_->enable_depth_test_);
        ImGui::WCombo("##bbmodesel", "Bounding box", debug_renderer_->bb_display_mode_, 3, bb_mode_items);
        ImGui::WCombo("##lightmodesel", "Light proxy", debug_renderer_->light_display_mode_, 3, light_mode_items);
        ImGui::SliderFloat("Wireframe", (float*)&geometry_renderer_->get_wireframe_mix_nc(), 0.0f, 1.0f);

        ImGui::Separator();
        if(ImGui::Button("Framebuffer Peek"))
        {
            framebuffer_peek = !framebuffer_peek;
        }
        ImGui::SameLine();
        if(ImGui::Button("Profile renderers"))
        {
            profile_renderers = !profile_renderers;
        }
    }

    // POST PROCESSING CONTROL
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Post-processing"))
    {
        ImGui::Text("Chromatic aberration");
        ImGui::SliderFloat("Shift", &post_processing_renderer_->aberration_shift_, 0.0f, 10.0f);
        ImGui::SliderFloat("Magnitude", &post_processing_renderer_->aberration_strength_, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Vignette");
        ImGui::SliderFloat("Falloff", &post_processing_renderer_->vignette_falloff_, 0.0f, 2.0f);
        ImGui::SliderFloat("Balance", &post_processing_renderer_->vignette_balance_, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Vibrance");
        ImGui::SliderFloat("Strength", &post_processing_renderer_->vibrance_, -1.0f, 1.0f);
        ImGui::SliderFloat3("Balance", (float*)&post_processing_renderer_->vibrance_bal_, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::SliderFloat("Saturation", &post_processing_renderer_->saturation_, 0.0f, 2.0f);
        ImGui::SliderFloat3("Gamma", (float*)&post_processing_renderer_->gamma_, 1.0f, 2.0f);
        ImGui::SliderFloat("Exposure", &post_processing_renderer_->exposure_, 0.1f, 5.0f);
        ImGui::SliderFloat("Contrast", &post_processing_renderer_->contrast_, 0.0f, 2.0f);

        ImGui::Separator();
        ImGui::Text("Fog");
        ImGui::Checkbox("Enable fog", &post_processing_renderer_->fog_enabled_);
        if(post_processing_renderer_->get_fog_enabled_flag())
        {
            ImGui::SliderFloat("Density", &post_processing_renderer_->fog_density_, 0.0f, 0.1f);
            ImGui::ColorEdit3("Color", (float*)&post_processing_renderer_->fog_color_);
        }

        ImGui::Separator();
        ImGui::Text("FXAA");
        ImGui::Checkbox("Enable FXAA", &post_processing_renderer_->fxaa_enabled_);
        if(post_processing_renderer_->get_fxaa_enabled_flag())
        {

        }

        ImGui::Separator();
        ImGui::Text("Accessibility");
        ImGui::WCombo("##daltmodesel", "Daltonize", post_processing_renderer_->acc_daltonize_mode_, 3, acc_dalt_mode_items);
        if(post_processing_renderer_->acc_daltonize_mode_)
        {
            ImGui::Indent();
            ImGui::WCombo("##blindnesssel", "Blindness", post_processing_renderer_->acc_blindness_type_, 3, acc_blindness_items);
            ImGui::Unindent();
        }

        ImGui::Separator();
        ImGui::Text("Misc.");
        ImGui::Checkbox("Enable dithering", &post_processing_renderer_->dithering_enabled_);
    }
    ImGui::End();

    // RENDERER STATISTICS
    if(profile_renderers)
    {
        if(!ImGui::Begin("Renderer profile"))
        {
            ImGui::End();
            return;
        }
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once); // will segfault if set to true (!)
        if(ImGui::CollapsingHeader("Render time"))
        {
            ImGui::PlotVar("Draw time", 1e3*last_render_time_, 0.0f, 16.66f);
            ImGui::PlotVar("Geometry pass", 1e3*geometry_dt_fifo_.last_element(), 0.0f, 16.66f);
            if(SSAO_renderer_->is_active())
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
        ImGui::End();
    }

    if(framebuffer_peek)
        debug_overlay_renderer_->generate_widget();
}
#endif //__DISABLE_EDITOR__

void RenderPipeline::render()
{
    #ifdef __PROFILE__
    float dt = 0.0f;
    std::chrono::nanoseconds period;
    #endif
// ------- DEFERRED PASS (no blending allowed) --------------------------------
// ------- GEOMETRY PASS (draw on texture "gbuffer") --------------------------
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        profile_clock_.restart();
        frame_clock_.restart();
    }
    #endif

        geometry_renderer_->render();

    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        geometry_dt_fifo_.push(dt);
    }
    #endif

// ------- SSAO (draw on texture "SSAObuffer") --------------------------------
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        profile_clock_.restart();
    }
    #endif

        SSAO_renderer_->render();

    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        SSAO_dt_fifo_.push(dt);
    }
    #endif

// ------- LIGHTING PASS (draw on texture "screen") ---------------------------
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        profile_clock_.restart();
    }
    #endif

        lighting_renderer_->render();

    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        lighting_dt_fifo_.push(dt);
    }
    #endif

// ------- FORWARD PASS (draw anything incompatible with deferred pass) -------
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        profile_clock_.restart();
    }
    #endif

        forward_renderer_->render();

    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        forward_dt_fifo_.push(dt);
    }
    #endif

// ------- BLUR PASS (draw on textures "bloom_i" with i in [0,3]) -------------
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        profile_clock_.restart();
    }
    #endif

    if(bloom_enabled_)
        bloom_renderer_->render();

    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        bloom_dt_fifo_.push(dt);
    }
    #endif

// ------- HDR, POST-PROCESSING (draw texture "screen" on actual screen) ------
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        profile_clock_.restart();
    }
    #endif

    post_processing_renderer_->render();

    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        period = profile_clock_.get_elapsed_time();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        pp_dt_fifo_.push(dt);
    }
    #endif

// ------- OVERLAY AND TEXT (draw to screen with alpha blending) --------------
        debug_renderer_->render();
        debug_overlay_renderer_->render();
        text_renderer_->render();

    #ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        period = frame_clock_.get_elapsed_time();
        last_render_time_ = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
    }
    #endif
}

void RenderPipeline::render_gui()
{
    gui_renderer_->render();    // Cursor...
}


void RenderPipeline::dbg_show_statistics()
{
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        FinalStatistics geom_stats = geometry_dt_fifo_.get_stats();
        FinalStatistics SSAO_stats = SSAO_dt_fifo_.get_stats();
        FinalStatistics lighting_stats = lighting_dt_fifo_.get_stats();
        FinalStatistics forward_stats = forward_dt_fifo_.get_stats();
        FinalStatistics bloom_stats = bloom_dt_fifo_.get_stats();
        FinalStatistics pp_stats = pp_dt_fifo_.get_stats();
        uint32_t n_iter = geometry_dt_fifo_.get_size();

        DLOGN("Geometry pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::LOW);
        geom_stats.debug_print(1e6, "µs", "profile");
        DLOGN("SSAO pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::LOW);
        SSAO_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Lighting pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::LOW);
        lighting_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Forward pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::LOW);
        forward_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Bloom pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::LOW);
        bloom_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Post-processing pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::LOW);
        pp_stats.debug_print(1e6, "µs", "profile");
    }
    #endif
}

}
