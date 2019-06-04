#include <cassert>

#include "debug_overlay_renderer.h"
#include "gfx_api.h"
#include "text_renderer.h"
#include "texture.h"
#include "math3d.h"
#include "logger.h"
#include "buffer_module.h"
#include "globals.h"
#include "scene.h"
#include "camera.h"
#include "config.h"
#include "png_loader.h"

#include "geometry_common.h"
#include "buffer_module.h"


#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;

DebugOverlayRenderer::DebugOverlayRenderer(TextRenderer& text_renderer):
peek_shader_(ShaderResource("fb_peek.vert;fb_peek.frag")),
render_target_("debugOverlayBuffer",
std::make_unique<Texture>(
    std::initializer_list<TextureUnitInfo>
    {
        TextureUnitInfo("debugOverlayTex"_h, TextureFilter::MIN_LINEAR, TextureIF::RGBA16F),
    },
    GLB.WIN_W,
    GLB.WIN_H,
    TextureWrap::CLAMP_TO_EDGE)),
current_pane_(0),
current_tex_(0),
raw_(false),
tone_map_(false),
show_r_(true),
show_g_(true),
show_b_(true),
invert_color_(false),
split_alpha_(true),
split_pos_(0.5f),
text_renderer_(text_renderer)
{
    enabled_ = false;

    //register_debug_pane(GMODULES::GET("backfaceDepthBuffer"_h));
    register_debug_pane(GMODULES::GET("gbuffer"_h));
    register_debug_pane(GMODULES::GET("shadowmap"_h));
    register_debug_pane(GMODULES::GET("SSAObuffer"_h));
    register_debug_pane(GMODULES::GET("SSRbuffer"_h));
    register_debug_pane(GMODULES::GET("bloombuffer"_h));
    register_debug_pane(GMODULES::GET("lbuffer"_h));
}

void DebugOverlayRenderer::register_debug_pane(std::vector<unsigned int>&& texture_indices,
                                               std::vector<std::string>&& sampler_names,
                                               std::vector<bool>&& is_depth)
{
    // Sanity check
    assert(texture_indices.size()==sampler_names.size()
           && "[DebugOverlayRenderer] Vector sizes don't match.");
    assert(sampler_names.size()==is_depth.size()
           && "[DebugOverlayRenderer] Vector sizes don't match.");

    // Register texture properties in a pane
    uint32_t n_tex = texture_indices.size();
    dbg::DebugPane dbg_pane;
    for(uint32_t ii=0; ii< n_tex; ++ii)
    {
        dbg_pane.push_back(dbg::DebugTextureProperties(texture_indices[ii],
                           sampler_names[ii],
                           is_depth[ii]));
    }
    debug_panes_.push_back(dbg_pane);
}

void DebugOverlayRenderer::register_debug_pane(BufferModule& buffer_module)
{
    uint32_t n_tex = buffer_module.get_num_textures();
    dbg::DebugPane dbg_pane;
    for(uint32_t ii=0; ii< n_tex; ++ii)
    {
        std::string sampler_name(HRESOLVE(buffer_module.get_texture().get_sampler_name(ii)));

        dbg_pane.push_back(dbg::DebugTextureProperties(buffer_module[ii],
                           sampler_name,
                           buffer_module.get_texture().is_depth(ii)));
    }
    debug_panes_.push_back(dbg_pane);
}

void DebugOverlayRenderer::render_pane(uint32_t index, Scene* pscene)
{
    // Clamp index
    index = (index>debug_panes_.size()-1)?debug_panes_.size()-1:index;

    // Calculate positioning
    size_t ntex = debug_panes_[index].size();
    float gap = 10.0f;
    float vpw = fmin((GLB.WIN_W-(ntex+1)*gap)/ntex, GLB.WIN_W/5.0f);
    float vph = fmin(vpw*GLB.WIN_W/GLB.WIN_H, GLB.WIN_H/5.0f);

    // Loop through registered textures
    for(int ii=0; ii<ntex; ++ii)
    {
        dbg::DebugTextureProperties& props = debug_panes_[index][ii];
        bool is_depth = props.is_depth;

        peek_shader_.send_uniform("b_isDepth"_h, is_depth);
        if(is_depth)
        {
            const mat4& P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
            vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));
            peek_shader_.send_uniform("v4_proj_params"_h, proj_params);
        }

        // TODO: don't use bind_texture2D
        Gfx::device->bind_texture2D(0, props.texture_index);
        Gfx::device->viewport((ii+1)*gap + ii*vpw, gap, vpw, vph);

        CGEOM.draw("quad"_h);

        text_renderer_.schedule_for_drawing(props.sampler_name,
                                            "arial"_h,
                                            (ii+1)*gap + ii*vpw,
                                            vph,
                                            1.0f,
                                            is_depth?vec3(1,0,0):vec3(1,1,1));
    }
}

void DebugOverlayRenderer::render_internal(Scene* pscene)
{
    // Get current texture properties
    dbg::DebugTextureProperties& props = debug_panes_[current_pane_][current_tex_];
    bool is_depth = props.is_depth;

    // Bind current texture as source and internal framebuffer as target
    render_target_.bind_as_target();
    // TODO: don't use bind_texture2D
    Gfx::device->bind_texture2D(0, props.texture_index);

    // Send uniforms to shader
    peek_shader_.use();
    peek_shader_.send_uniform<int>("screenTex"_h, 0);

    peek_shader_.send_uniform("b_toneMap"_h, tone_map_);
    peek_shader_.send_uniform("b_isDepth"_h, is_depth);
    peek_shader_.send_uniform("b_splitAlpha"_h, split_alpha_);
    peek_shader_.send_uniform("b_invert"_h, invert_color_);
    peek_shader_.send_uniform("f_splitPos"_h, split_pos_);
    peek_shader_.send_uniform("v3_channelFilter"_h, vec3((float)show_r_, (float)show_g_, (float)show_b_));
    peek_shader_.send_uniform("v2_texelSize"_h, vec2(1.0f/render_target_.get_width(),1.0f/render_target_.get_height()));

    // If texture is a depth buffer, more information needed
    if(is_depth)
    {
        const mat4& P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
        vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));
        peek_shader_.send_uniform("v4_proj_params"_h, proj_params);
    }

    // Draw
    Gfx::device->clear(CLEAR_COLOR_FLAG);

    CGEOM.draw("quad"_h);

    peek_shader_.unuse();
    render_target_.unbind_as_target();
    Gfx::device->flush();
}

void DebugOverlayRenderer::render(Scene* pscene)
{
    peek_shader_.use();
    render_pane(current_pane_, pscene);
    peek_shader_.unuse();
}

bool DebugOverlayRenderer::save_fb_to_image(const std::string& filename)
{
    // Allocate buffer for image data
    int width = render_target_.get_width();
    int height = render_target_.get_height();
    int img_size = width * height * 4;
    unsigned char* pixels = new unsigned char[img_size];

    // Bind framebuffer, change alignment to 1 to avoid out of bounds writes,
    // read framebuffer to pixel array and unbind
    render_target_.bind_as_target();
    Gfx::device->read_framebuffer_rgba(width, height, pixels);
    render_target_.unbind_as_target();

    // Save to PNG image
    bool success = false;
    fs::path file_path;
    if(CONFIG.get("root.folders.log"_h, file_path))
    {
        file_path /= filename;
        PngLoader png_loader;
        png_loader.write_png(file_path, pixels, width, height);
        success = true;
    }

    delete[] pixels;
    return success;
}

#ifndef __DISABLE_EDITOR__
static bool save_image = false;

void DebugOverlayRenderer::framebuffer_peek_widget(Scene* pscene)
{
    if(!ImGui::Begin("Framebuffer peek"))
    {
        ImGui::End();
        return;
    }

    // * Get render properties from GUI
    ImGui::BeginChild("##peekctl", ImVec2(0, 3*ImGui::GetItemsLineHeightWithSpacing()));
    ImGui::Columns(2, nullptr, false);

    if(ImGui::SliderInt("Panel", &current_pane_, 0, debug_panes_.size()-1))
    {
        current_tex_ = 0;
    }
    int ntex = debug_panes_[current_pane_].size();
    ImGui::SliderInt("Texture", &current_tex_, 0, ntex-1);
    dbg::DebugTextureProperties& props = debug_panes_[current_pane_][current_tex_];
    ImGui::Text("name: %s", props.sampler_name.c_str());

    ImGui::NextColumn();
    ImGui::Checkbox("Tone mapping", &tone_map_);
    ImGui::SameLine(); ImGui::Checkbox("R##0", &show_r_);
    ImGui::SameLine(); ImGui::Checkbox("G##0", &show_g_);
    ImGui::SameLine(); ImGui::Checkbox("B##0", &show_b_);
    ImGui::SameLine(); ImGui::Checkbox("Invert", &invert_color_);
    ImGui::SameLine(); ImGui::Checkbox("Raw", &raw_);
    ImGui::SameLine();
    if(ImGui::Button("Save to file"))
        save_image = true;


    ImGui::Checkbox("Alpha split", &split_alpha_);
    if(split_alpha_)
    {
        ImGui::SameLine();
        ImGui::SliderFloat("Split pos.", &split_pos_, 0.f, 1.f);
    }

    ImGui::EndChild();

    // * Render current texture offscreen
    if(!raw_)
        render_internal(pscene);

    // * Show image in window
    float winx = std::max(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 8.f, 0.f);
    float winy = std::max(ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - 8.f, 0.f);

    if(raw_)
    {
        ImGui::GetWindowDrawList()->AddImage((void*)(uint64_t)props.texture_index,
                                             ImGui::GetCursorScreenPos(),
                                             ImVec2(winx, winy),
                                             ImVec2(0, 1), ImVec2(1, 0));
    }
    else
    {
        uint64_t target_id = render_target_.get_texture()[0];
        ImGui::GetWindowDrawList()->AddImage((void*)target_id,
                                             ImGui::GetCursorScreenPos(),
                                             ImVec2(winx, winy),
                                             ImVec2(0, 1), ImVec2(1, 0));
    }

    // * Save image if needed
    if(save_image && !raw_) // TMP only handle FB save for now
    {
        Gfx::device->finish();
        std::string filename = props.sampler_name + "_" + std::to_string(props.texture_index) + ".png";
        if(save_fb_to_image(filename))
            DLOGN("[DebugOverlayRenderer] Saved engine texture to file:", "core");
        else
            DLOGE("[DebugOverlayRenderer] Unable to save engine texture to file:", "core");
        DLOGI("<p>" + filename + "</p>", "core");
        save_image = false;
    }

    ImGui::End();
}
#endif

}
