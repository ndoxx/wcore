#include "pipeline.h"

#include "globals.h"
#include "geometry_renderer.h"
#include "lighting_renderer.h"
#include "forward_renderer.h"
#include "bloom_renderer.h"
#include "SSAO_renderer.h"
#include "SSR_renderer.h"
#include "post_processing_renderer.h"
#include "text_renderer.h"
#include "debug_renderer.h"
#include "debug_overlay_renderer.h"
#include "shadow_map_renderer.h"
#include "gui_renderer.h"

#include "geometry_common.h"
#include "debug_info.h"
#include "logger.h"
#include "input_handler.h"
#include "scene.h"
#include "shader.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
    #include "editor_tweaks.h"
#endif

#ifdef __PROFILE__
    #include "clock.hpp"
    #include "moving_average.h"
    #include "gfx_driver.h"
    #define PROFILING_MAX_SAMPLES 1000
#endif

namespace wcore
{

RenderPipeline::RenderPipeline()
{
    /*GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "backfaceDepthBuffer",
        std::make_shared<Texture>(
            std::vector<hash_t>{"backfaceDepthTex"_h},
            std::vector<GLenum>{ GL_NEAREST},
            std::vector<GLenum>{ GL_DEPTH_COMPONENT32},
            std::vector<GLenum>{ GL_DEPTH_COMPONENT},
            GLB.WIN_W,
            GLB.WIN_H,
            GL_TEXTURE_2D,
            true),
        std::vector<GLenum>({GL_DEPTH_ATTACHMENT})
    ));*/

    // Buffers with facilities for Geometry pass
    GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "gbuffer",
        std::make_shared<Texture>(
            std::vector<hash_t>{"normalTex"_h, "albedoTex"_h, "depthTex"_h},
            std::vector<GLenum>{GL_NEAREST,      GL_NEAREST,   GL_NEAREST},
            std::vector<GLenum>{GL_RGBA16_SNORM, GL_RGBA,      GL_DEPTH_COMPONENT32},
            std::vector<GLenum>{GL_RGBA,         GL_RGBA,      GL_DEPTH_COMPONENT},
            GLB.WIN_W,
            GLB.WIN_H,
            GL_TEXTURE_2D,
            true),
        std::vector<GLenum>({GL_COLOR_ATTACHMENT0,
                             GL_COLOR_ATTACHMENT1,
                             GL_DEPTH_ATTACHMENT})
    ));

    // Buffer with facilities for Lighting pass
    GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "lbuffer",
        std::make_shared<Texture>(
            std::vector<hash_t>{"screenTex"_h, "brightTex"_h,         "ldepthStencilTex"_h},
            std::vector<GLenum> {GL_NEAREST,     GL_LINEAR_MIPMAP_LINEAR, GL_NONE},
            std::vector<GLenum> {GL_RGB16F,      GL_RGB,                  GL_DEPTH24_STENCIL8},
            std::vector<GLenum> {GL_RGB,         GL_RGB,                  GL_DEPTH_STENCIL},
            GLB.WIN_W,            // brightTex will contain the bright map.
            GLB.WIN_H,           // We use the multiple render target scheme
            GL_TEXTURE_2D,          // to populate this texture during the
            true,                   // lighting pass.
            true), // Lazy mipmap initialization needed
        std::vector<GLenum>({GL_COLOR_ATTACHMENT0,
                             GL_COLOR_ATTACHMENT1,
                             GL_DEPTH_STENCIL_ATTACHMENT})
    ));

    // Buffer for SSAO
    GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "SSAObuffer",
        std::make_shared<Texture>(
            std::vector<hash_t>{"SSAOTex"_h},
            std::vector<GLenum>{GL_LINEAR},
            std::vector<GLenum>{GL_R8},
            std::vector<GLenum>{GL_RED},
            GLB.WIN_W/2,
            GLB.WIN_H/2,
            GL_TEXTURE_2D,
            true),
        std::vector<GLenum>({GL_COLOR_ATTACHMENT0})
    ));

    // Buffer for SSR
    GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "SSRbuffer",
        std::make_shared<Texture>(
            std::vector<hash_t>{"SSRTex"_h},
            std::vector<GLenum>{GL_LINEAR},
            std::vector<GLenum>{GL_RGBA16F},
            std::vector<GLenum>{GL_RGBA},
            GLB.WIN_W/2,
            GLB.WIN_H/2,
            GL_TEXTURE_2D,
            true),
        std::vector<GLenum>({GL_COLOR_ATTACHMENT0})
    ));

    // Common utility meshes
    GeometryCommon::Instance();

    geometry_renderer_        = new GeometryRenderer();
    shadow_map_renderer_      = new ShadowMapRenderer();
    lighting_renderer_        = new LightingRenderer();
    forward_renderer_         = new ForwardRenderer();
    SSAO_renderer_            = new SSAORenderer();
    SSR_renderer_             = new SSRRenderer();
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
    delete gui_renderer_;
    delete debug_overlay_renderer_;
    delete debug_renderer_;
    delete text_renderer_;
    delete post_processing_renderer_;
    delete bloom_renderer_;
    delete SSR_renderer_;
    delete SSAO_renderer_;
    delete forward_renderer_;
    delete lighting_renderer_;
    delete shadow_map_renderer_;
    delete geometry_renderer_;

    GeometryCommon::Kill();
}

void RenderPipeline::init_events(InputHandler& handler)
{
    subscribe("input.mouse.unlocked"_h, handler, &RenderPipeline::onMouseEvent);
    subscribe("input.keyboard"_h, handler, &RenderPipeline::onKeyboardEvent);
}

static int SSAO_kernel_half_size = 3;
static float SSAO_sigma = 1.8f;
static int bloom_kernel_half_size = 3;
static float bloom_sigma = 1.8f;
static bool framebuffer_peek = false;

void RenderPipeline::init_self()
{
#ifndef __DISABLE_EDITOR__
    auto* edtweaks = locate_init<EditorTweaksInitializer>("EditorTweaks"_h);

    // Pipeline control tweaks
    edtweaks->register_variable("root.geometry.parallax.min_distance"_h,   geometry_renderer_->get_min_parallax_distance_nc());
    // Post-processing tweaks
    edtweaks->register_variable("root.postproc.aberration.shift"_h,        post_processing_renderer_->get_aberration_shift_nc());
    edtweaks->register_variable("root.postproc.aberration.magnitude"_h,    post_processing_renderer_->get_aberration_strength_nc());
    edtweaks->register_variable("root.postproc.vignette.falloff"_h,        post_processing_renderer_->get_vignette_falloff_nc());
    edtweaks->register_variable("root.postproc.vignette.balance"_h,        post_processing_renderer_->get_vignette_balance_nc());
    edtweaks->register_variable("root.postproc.color.vibrance.strength"_h, post_processing_renderer_->get_vibrance_nc());
    edtweaks->register_variable("root.postproc.color.vibrance.balance"_h,  post_processing_renderer_->get_vibrance_balance_nc());
    edtweaks->register_variable("root.postproc.fxaa.enabled"_h,            post_processing_renderer_->get_fxaa_enabled_nc());
    edtweaks->register_variable("root.postproc.dithering.enabled"_h,       post_processing_renderer_->get_dithering_enabled_nc());

    // SSAO tweaks
    edtweaks->register_variable("root.ssao.radius"_h,                      SSAO_renderer_->get_radius_nc());
    edtweaks->register_variable("root.ssao.bias"_h,                        SSAO_renderer_->get_scalar_bias_nc());
    edtweaks->register_variable("root.ssao.vbias"_h,                       SSAO_renderer_->get_vector_bias_nc());
    edtweaks->register_variable("root.ssao.intensity"_h,                   SSAO_renderer_->get_intensity_nc());
    edtweaks->register_variable("root.ssao.scale"_h,                       SSAO_renderer_->get_scale_nc());
    edtweaks->register_variable("root.ssao.blur.passes"_h,                 SSAO_renderer_->get_blur_policy_nc().n_pass_);
    edtweaks->register_variable("root.ssao.blur.compression"_h,            SSAO_renderer_->get_blur_policy_nc().gamma_r_);
    edtweaks->register_variable("root.ssao.blur.kernel_half_size"_h,       SSAO_kernel_half_size);
    edtweaks->register_variable("root.ssao.blur.kernel_sigma"_h,           SSAO_sigma);
#endif
}

#ifdef __DEBUG__
void RenderPipeline::perform_test()
{

}
#endif

void RenderPipeline::set_shadow_mapping_enabled(bool value)    { lighting_renderer_->set_shadow_mapping_enabled(value);
                                                                 shadow_map_renderer_->set_enabled(value); }
void RenderPipeline::set_directional_light_enabled(bool value) { lighting_renderer_->set_directional_light_enabled(value); }
void RenderPipeline::set_shadow_bias(float value)              { lighting_renderer_->set_shadow_bias(value); }
void RenderPipeline::set_bright_threshold(float value)         { lighting_renderer_->set_bright_threshold(value); }
void RenderPipeline::set_bright_knee(float value)              { lighting_renderer_->set_bright_knee(value); }
void RenderPipeline::set_shadow_slope_bias(float value)        { lighting_renderer_->set_shadow_slope_bias(value); }
void RenderPipeline::set_normal_offset(float value)            { shadow_map_renderer_->set_normal_offset(value); }

void RenderPipeline::set_bloom_enabled(bool value)             { post_processing_renderer_->set_bloom_enabled(value); }
void RenderPipeline::set_fog_enabled(bool value)               { post_processing_renderer_->set_fog_enabled(value); }
void RenderPipeline::set_fxaa_enabled(bool value)              { post_processing_renderer_->set_fxaa_enabled(value); }
void RenderPipeline::set_pp_gamma(const math::vec3& value)     { post_processing_renderer_->set_gamma(value); }
void RenderPipeline::set_pp_fog_color(const math::vec3& value) { post_processing_renderer_->set_fog_color(value); }
void RenderPipeline::set_pp_saturation(float value)            { post_processing_renderer_->set_saturation(value); }
void RenderPipeline::set_pp_fog_density(float value)           { post_processing_renderer_->set_fog_density(value); }
void RenderPipeline::set_pp_exposure(float value)              { post_processing_renderer_->set_exposure(value); }
void RenderPipeline::set_pp_contrast(float value)              { post_processing_renderer_->set_contrast(value); }
void RenderPipeline::set_pp_vibrance(float value)              { post_processing_renderer_->set_vibrance(value); }
void RenderPipeline::set_pp_vignette_falloff(float value)      { post_processing_renderer_->set_vignette_falloff(value); }
void RenderPipeline::set_pp_vignette_balance(float value)      { post_processing_renderer_->set_vignette_balance(value); }
void RenderPipeline::set_pp_aberration_shift(float value)      { post_processing_renderer_->set_aberration_shift(value); }
void RenderPipeline::set_pp_aberration_strength(float value)   { post_processing_renderer_->set_aberration_strength(value); }
void RenderPipeline::set_pp_acc_blindness_type(int value)      { post_processing_renderer_->set_acc_blindness_type(value); }
void RenderPipeline::set_pp_vibrance_balance(const math::vec3& value) { post_processing_renderer_->set_vibrance_balance(value); }

static float neighbors_search_eadius = 5.f;
static bool HOTSWAP_SHADERS = false;

bool RenderPipeline::onKeyboardEvent(const WData& data)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
        case "k_tg_editor"_h:
            gui_renderer_->toggle_cursor();
            break;
        case "k_tg_fog"_h:
    		post_processing_renderer_->toggle_fog();
    		break;
        case "k_bb_next_mode"_h:
    		debug_renderer_->next_bb_display_mode();
    		break;
        case "k_tg_line_models"_h:
    		debug_renderer_->toggle_line_models();
    		break;
        case "k_light_volumes_next_mode"_h:
    		debug_renderer_->next_light_display_mode();
    		break;
        case "k_tg_debug_overlay"_h:
    		debug_overlay_renderer_->toggle();
    		break;
        case "k_debug_overlay_next_mode"_h:
    		debug_overlay_renderer_->next_pane();
    		break;
        case "k_tg_debug_info"_h:
    		DINFO.toggle();
    		break;
        case "k_tg_wireframe"_h:
    		geometry_renderer_->toggle_wireframe();
    		break;
#ifdef __DEBUG__
        case "k_hotswap_shaders"_h:
            HOTSWAP_SHADERS = true;
            break;
        case "k_test_key"_h:
            perform_test();
            break;
#endif
        case "k_show_neighbors"_h:
            debug_renderer_->show_selection_neighbors(locate<Scene>("Scene"_h), neighbors_search_eadius);
            break;
    }

    return true; // Do NOT consume event
}

bool RenderPipeline::onMouseEvent(const WData& data)
{
    const MouseData& md = static_cast<const MouseData&>(data);
    gui_renderer_->set_cursor_position(md.dx, md.dy);

    return true; // Do NOT consume event
}

#ifdef __PROFILE__
static bool profile_renderers = false;
static uint32_t frame_cnt = 0;
static float last_render_time_ = 0.0f;
static nanoClock frame_clock_;
#endif

#ifndef __DISABLE_EDITOR__
const char* bb_mode_items[]       = {"Hidden", "AABB", "OBB", "Origin"};
const char* light_mode_items[]    = {"Hidden", "Location", "Scaled"};
const char* acc_dalt_mode_items[] = {"Off", "Simulate", "Apply correction"};
const char* acc_blindness_items[] = {"Protanopia", "Deuteranopia", "Tritanopia"};

void RenderPipeline::generate_widget()
{
    Scene* pscene = locate<Scene>("Scene"_h);

    // DEBUG DRAWING
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Debug drawing"))
    {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Primitives"))
        {
            ImGui::Checkbox("Depth test", &debug_renderer_->enable_depth_test_);
            ImGui::WCombo("##bbmodesel", "Bounding box", debug_renderer_->bb_display_mode_, 4, bb_mode_items);
            ImGui::WCombo("##lightmodesel", "Light proxy", debug_renderer_->light_display_mode_, 3, light_mode_items);
            ImGui::SliderFloat("Wireframe", (float*)&geometry_renderer_->get_wireframe_mix_nc(), 0.0f, 1.0f);
            if(ImGui::Button("Clear draw requests"))
            {
                debug_renderer_->clear_draw_requests();
            }
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Raycast"))
        {
            ImGui::Checkbox("Show rays", &locate<RayCaster>("RayCaster"_h)->get_show_ray_nc());
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Scene graph"))
        {
            ImGui::Checkbox("Show static octree", &debug_renderer_->show_static_octree_);
            ImGui::SliderFloat("Sel. half-bnd",   &neighbors_search_eadius, 0.5f, 10.0f);
            if(ImGui::Button("Show selection neighbors"))
            {
                debug_renderer_->show_selection_neighbors(pscene, neighbors_search_eadius);
            }
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Windows"))
        {
            if(ImGui::Button("Framebuffer peek"))
            {
                framebuffer_peek = !framebuffer_peek;
            }
#ifdef __PROFILE__
            ImGui::SameLine();
            if(ImGui::Button("Profile renderers"))
            {
                profile_renderers = !profile_renderers;
                Renderer::PROFILING_ACTIVE = profile_renderers;
            }
#endif
            ImGui::TreePop();
        }
    }

    // PIPELINE CONTROL SECTION
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Pipeline control"))
    {
        ImGui::BeginChild("##pipelinectl", ImVec2(0, 3*ImGui::GetItemsLineHeightWithSpacing()));
        ImGui::Columns(2, nullptr, false);
        ImGui::Checkbox("Lighting",       &lighting_renderer_->get_lighting_enabled_nc());
        ImGui::Checkbox("Shadow Mapping", &lighting_renderer_->get_shadow_enabled_nc());
        if(ImGui::Checkbox("SSAO", &SSAO_renderer_->get_enabled()))
        {
            lighting_renderer_->set_SSAO_enabled(SSAO_renderer_->is_enabled());
        }
        ImGui::NextColumn();
        if(ImGui::Checkbox("SSR", &SSR_renderer_->get_enabled()))
        {
            //lighting_renderer_->set_SSR_enabled(SSR_renderer_->is_enabled());
        }
        if(ImGui::Checkbox("Bloom", &bloom_renderer_->get_enabled()))
        {
            post_processing_renderer_->set_bloom_enabled(bloom_renderer_->is_enabled());
        }
        ImGui::Checkbox("Forward pass", &forward_renderer_->get_enabled());
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Text("Parallax mapping");
        ImGui::SliderFloat("min distance", &geometry_renderer_->get_min_parallax_distance_nc(), 0.0f, 100.0f);
    }

    // SSAO OPTIONS
    if(SSAO_renderer_->is_enabled())
    {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("SSAO control"))
        {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode("Tuning"))
            {
                ImGui::SliderFloat("Radius",      &SSAO_renderer_->get_radius_nc(), 0.01f, 1.0f);
                ImGui::SliderFloat("Scalar bias", &SSAO_renderer_->get_scalar_bias_nc(), 0.0f, 1.0f);
                ImGui::SliderFloat("Vector bias", &SSAO_renderer_->get_vector_bias_nc(), 0.0f, 0.5f);
                ImGui::SliderFloat("Intensity",   &SSAO_renderer_->get_intensity_nc(), 0.0f, 5.0f);
                ImGui::SliderFloat("Scale",       &SSAO_renderer_->get_scale_nc(), 0.01f, 1.0f);
                ImGui::TreePop();
                ImGui::Separator();
            }

            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("Blur"))
            {
                int ker_size = 2*SSAO_kernel_half_size-1;
                ImGui::Text("Blur: Gaussian kernel %dx%d", ker_size, ker_size);
                bool update_kernel = ImGui::SliderInt("Half-size", &SSAO_kernel_half_size, 3, 8);
                update_kernel     |= ImGui::SliderFloat("Sigma",   &SSAO_sigma, 0.5f, 2.0f);
                if(update_kernel)
                {
                    SSAO_renderer_->get_blur_policy_nc().kernel_.update_kernel(2*SSAO_kernel_half_size-1, SSAO_sigma);
                }

                ImGui::SliderInt("Blur passes",   &SSAO_renderer_->get_blur_policy_nc().n_pass_, 0, 5);
                ImGui::SliderFloat("Compression", &SSAO_renderer_->get_blur_policy_nc().gamma_r_, 0.5f, 2.0f);
                ImGui::TreePop();
            }
        }
    }

    // SSR OPTIONS
    if(SSR_renderer_->is_enabled())
    {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("SSR control"))
        {
            ImGui::SliderInt("Max ray steps", &SSR_renderer_->get_ray_steps(), 0, 32);
            ImGui::SliderInt("Max b-search steps", &SSR_renderer_->get_bin_steps(), 0, 32);
            ImGui::SliderFloat("Max ray dist", &SSR_renderer_->get_max_ray_distance(), 0.0f, 50.0f);
            ImGui::SliderFloat("Px stride", &SSR_renderer_->get_pix_stride(), 1.0f, 20.0f);
            ImGui::SliderFloat("Px stride cuttoff", &SSR_renderer_->get_pix_stride_cuttoff(), 0.0f, 200.0f);
            ImGui::SliderFloat("Px thickness", &SSR_renderer_->get_pix_thickness(), 0.0f, 5.0f);
            ImGui::SliderFloat("Min gloss", &SSR_renderer_->get_min_glossiness(), 0.0f, 1.0f);
            ImGui::SliderFloat("Dither amt.", &SSR_renderer_->get_dither_amount(), 0.0f, 1.0f);
            ImGui::SliderFloat("Fade eye start", &SSR_renderer_->get_fade_eye_start(), 0.0f, 1.0f);
            ImGui::SliderFloat("Fade eye end", &SSR_renderer_->get_fade_eye_end(), 0.0f, 1.0f);
            ImGui::SliderFloat("Fade screen edge", &SSR_renderer_->get_fade_screen_edge(), 0.0f, 1.0f);
            ImGui::SliderFloat("DBG Probe", &SSR_renderer_->get_probe(), 0.0f, 1.0f);
            ImGui::Separator();

            ImGui::Checkbox("Enable blur", &SSR_renderer_->get_blur_enabled());
        }
    }

    // BLOOM OPTIONS
    if(bloom_renderer_->is_enabled())
    {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("Bloom control"))
        {
            ImGui::SliderFloat("Threshold", &lighting_renderer_->get_bright_threshold_nc(), 0.5f, 2.0f);
            ImGui::SliderFloat("Knee",      &lighting_renderer_->get_bright_knee_nc(), 0.01f, 1.0f);
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
    if(lighting_renderer_->get_shadow_enabled_nc())
    {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("Shadow control"))
        {
            ImGui::SliderFloat("Depth bias",    &lighting_renderer_->get_shadow_bias_nc(), 0.0f, 5.0f);
            ImGui::SliderFloat("Slope bias",    &lighting_renderer_->get_shadow_slope_bias_nc(), 0.0f, 0.5f);
            ImGui::SliderFloat("Normal offset", &shadow_map_renderer_->get_normal_offset(), -1.0f, 1.0f);
        }
    }

    // POST PROCESSING CONTROL
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Post-processing"))
    {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Chromatic aberration"))
        {
            ImGui::SliderFloat("Shift",     &post_processing_renderer_->get_aberration_shift_nc(), 0.0f, 10.0f);
            ImGui::SliderFloat("Magnitude", &post_processing_renderer_->get_aberration_strength_nc(), 0.0f, 1.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Vignette"))
        {
            ImGui::SliderFloat("Falloff", &post_processing_renderer_->get_vignette_falloff_nc(), 0.0f, 2.0f);
            ImGui::SliderFloat("Balance", &post_processing_renderer_->get_vignette_balance_nc(), 0.0f, 1.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Color"))
        {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode("Vibrance"))
            {
                ImGui::SliderFloat("Strength",         &post_processing_renderer_->get_vibrance_nc(), -1.0f, 1.0f);
                ImGui::SliderFloat3("Balance", (float*)&post_processing_renderer_->get_vibrance_balance_nc(), 0.0f, 1.0f);
                ImGui::TreePop();
                ImGui::Separator();
            }

            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode("Correction"))
            {
                ImGui::SliderFloat("Saturation",     &post_processing_renderer_->get_saturation_nc(), 0.0f, 2.0f);
                ImGui::SliderFloat3("Gamma", (float*)&post_processing_renderer_->get_gamma_nc(), 1.0f, 2.0f);
                ImGui::SliderFloat("Exposure",       &post_processing_renderer_->get_exposure_nc(), 0.1f, 5.0f);
                ImGui::SliderFloat("Contrast",       &post_processing_renderer_->get_contrast_nc(), 0.0f, 2.0f);
                ImGui::TreePop();
                ImGui::Separator();
            }
            ImGui::TreePop();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Fog"))
        {
            ImGui::Checkbox("Enable fog", &post_processing_renderer_->get_fog_enabled_nc());
            if(post_processing_renderer_->get_fog_enabled_nc())
            {
                ImGui::SliderFloat("Density",      &post_processing_renderer_->get_fog_density_nc(), 0.0f, 0.1f);
                ImGui::ColorEdit3("Color", (float*)&post_processing_renderer_->get_fog_color_nc());
            }
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("FXAA"))
        {
            ImGui::Checkbox("Enable FXAA", &post_processing_renderer_->get_fxaa_enabled_nc());
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Accessibility"))
        {
            ImGui::WCombo("##daltmodesel", "Daltonize", post_processing_renderer_->get_acc_daltonize_mode_nc(), 3, acc_dalt_mode_items);
            if(post_processing_renderer_->get_acc_daltonize_mode_nc())
            {
                ImGui::Indent();
                ImGui::WCombo("##blindnesssel", "Blindness", post_processing_renderer_->get_acc_blindness_type_nc(), 3, acc_blindness_items);
                ImGui::Unindent();
            }
            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
        if(ImGui::TreeNode("Misc."))
        {
            ImGui::Checkbox("Enable dithering", &post_processing_renderer_->get_dithering_enabled_nc());
            ImGui::TreePop();
        }
    }

    // RENDERER STATISTICS
#ifdef __PROFILE__
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
            ImGui::PlotVar("Geometry pass", geometry_renderer_->last_dt(), 0.0f, 16.66f);
            if(SSAO_renderer_->is_enabled())
                ImGui::PlotVar("SSAO", 1e3*SSAO_renderer_->last_dt(), 0.0f, 16.66f);
            if(SSR_renderer_->is_enabled())
                ImGui::PlotVar("SSR", 1e3*SSR_renderer_->last_dt(), 0.0f, 16.66f);
            if(shadow_map_renderer_->is_enabled())
                ImGui::PlotVar("Shadow mapping", 1e3*shadow_map_renderer_->last_dt(), 0.0f, 16.66f);
            ImGui::PlotVar("Lighting pass", 1e3*lighting_renderer_->last_dt(), 0.0f, 16.66f);
            ImGui::PlotVar("Forward pass", 1e3*forward_renderer_->last_dt(), 0.0f, 16.66f);
            if(bloom_renderer_->is_enabled())
                ImGui::PlotVar("Bloom pass", 1e3*bloom_renderer_->last_dt(), 0.0f, 16.66f);
            ImGui::PlotVar("Post processing", 1e3*post_processing_renderer_->last_dt(), 0.0f, 16.66f);

            if(++frame_cnt>200)
            {
                frame_cnt = 0;
                ImGui::PlotVarFlushOldEntries();
            }
        }
        ImGui::End();
    }
#endif
    if(framebuffer_peek)
        debug_overlay_renderer_->framebuffer_peek_widget(pscene);
}
#endif //__DISABLE_EDITOR__

void RenderPipeline::render()
{
    Scene* pscene = locate<Scene>("Scene"_h);

#ifdef __DEBUG__
    // Reload shaders at start of frame
    if(HOTSWAP_SHADERS)
    {
        GFX::finish();
        Shader::dbg_hotswap();
        GFX::finish();
        HOTSWAP_SHADERS = false;
    }
#endif

#ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        frame_clock_.restart();
    }
#endif

    geometry_renderer_->Render(pscene);
    SSAO_renderer_->Render(pscene);
    SSR_renderer_->Render(pscene);
    shadow_map_renderer_->Render(pscene);
    lighting_renderer_->Render(pscene);
    bloom_renderer_->Render(pscene);
    forward_renderer_->Render(pscene);
    post_processing_renderer_->Render(pscene);
    debug_renderer_->Render(pscene);
    debug_overlay_renderer_->Render(pscene);
    text_renderer_->Render(pscene);

#ifdef __PROFILE__
    if(profile_renderers)
    {
        GFX::finish();
        std::chrono::nanoseconds period = frame_clock_.get_elapsed_time();
        last_render_time_ = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
    }
#endif
}

void RenderPipeline::render_gui()
{
    Scene* pscene = locate<Scene>("Scene"_h);
    gui_renderer_->render(pscene);    // Cursor...
}


void RenderPipeline::dbg_show_statistics()
{
    #ifdef __PROFILE__
    if(profile_renderers)
    {
        FinalStatistics geom_stats = geometry_renderer_->get_stats();
        FinalStatistics SSAO_stats = SSAO_renderer_->get_stats();
        FinalStatistics shadow_stats = shadow_map_renderer_->get_stats();
        FinalStatistics lighting_stats = lighting_renderer_->get_stats();
        FinalStatistics forward_stats = forward_renderer_->get_stats();
        FinalStatistics bloom_stats = bloom_renderer_->get_stats();
        FinalStatistics pp_stats = post_processing_renderer_->get_stats();
        uint32_t n_iter = geom_stats.npoints;

        DLOGN("Geometry pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
        geom_stats.debug_print(1e6, "µs", "profile");
        DLOGN("SSAO pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
        SSAO_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Shadow pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
        shadow_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Lighting pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
        lighting_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Forward pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
        forward_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Bloom pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
        bloom_stats.debug_print(1e6, "µs", "profile");
        DLOGN("Post-processing pass statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
        pp_stats.debug_print(1e6, "µs", "profile");
    }
    #endif
}

#ifdef __DEBUG__
void RenderPipeline::show_light_proxy(int mode, float scale)
{
    debug_renderer_->set_light_display_mode(mode);
    debug_renderer_->set_light_proxy_scale(scale);
}

void RenderPipeline::debug_draw_segment(const math::vec3& world_start,
                                        const math::vec3& world_end,
                                        int ttl,
                                        const math::vec3& color)
{
    debug_renderer_->request_draw_segment(world_start, world_end, ttl, color);
}
void RenderPipeline::debug_draw_sphere(const math::vec3& world_pos,
                                       float radius,
                                       int ttl,
                                       const math::vec3& color)
{
    debug_renderer_->request_draw_sphere(world_pos, radius, ttl, color);
}
void RenderPipeline::debug_draw_cross3(const math::vec3& world_pos,
                                       float radius,
                                       int ttl,
                                       const math::vec3& color)
{
    debug_renderer_->request_draw_cross3(world_pos, radius, ttl, color);
}
#endif

}
