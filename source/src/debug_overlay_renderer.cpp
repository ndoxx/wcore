#include <cassert>

#include "debug_overlay_renderer.h"
#include "gfx_driver.h"
#include "text_renderer.h"
#include "texture.h"
#include "math3d.h"
#include "logger.h"
#include "buffer_module.h"
#include "globals.h"
#include "scene.h"
#include "camera.h"

#include "g_buffer.h"
#include "l_buffer.h"
#include "SSAO_buffer.h"
#include "SSR_buffer.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;

DebugOverlayRenderer::DebugOverlayRenderer(TextRenderer& text_renderer):
Renderer<Vertex3P>(),
passthrough_shader_(ShaderResource("passthrough.vert;passthrough.frag")),
mode_(0),
active_(false),
text_renderer_(text_renderer)
{
    load_geometry();

    auto pgbuffer = Texture::get_named_texture("gbuffer"_h).lock();
    auto plbuffer = Texture::get_named_texture("lbuffer"_h).lock();
    auto psbuffer = Texture::get_named_texture("shadowmap"_h).lock();
    auto pbloom   = Texture::get_named_texture("bloom"_h).lock();
    auto pssao    = Texture::get_named_texture("SSAObuffer"_h).lock();
    auto pssr     = Texture::get_named_texture("SSRbuffer"_h).lock();
    auto pssrblur = Texture::get_named_texture("SSRBlurBuffer"_h).lock();

    register_debug_pane(GBuffer::Instance());
    //register_debug_pane(BackFaceDepthBuffer::Instance());

    register_debug_pane({(*psbuffer)[0]},
                        {"shadowTex"},
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
                        {false}
#else
                        {true}
#endif
                        );

    register_debug_pane(LBuffer::Instance());
    register_debug_pane(SSAOBuffer::Instance());
    register_debug_pane(SSRBuffer::Instance());

    /*register_debug_pane({(*pssr)[0], (*pssrblur)[0]},
                        {"SSR", "SSR_hblur"},
                        {false, false});*/

    register_debug_pane({(*plbuffer)[1], (*pbloom)[0]},
                        {"screenTex", "bloomTex"},
                        {false, false});
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

        dbg_pane.push_back(dbg::DebugTextureProperties(buffer_module[ii],
                           std::to_string(buffer_module.get_texture()->get_sampler_name(ii)),
                           buffer_module.get_texture()->is_depth(ii)));
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

        passthrough_shader_.send_uniform("b_isDepth"_h, is_depth);
        if(is_depth)
        {
            passthrough_shader_.send_uniform("f_near"_h, pscene->get_camera().get_near());
            passthrough_shader_.send_uniform("f_far"_h, pscene->get_camera().get_far());
        }

        GFX::bind_texture2D(0, props.texture_index);
        GFX::viewport((ii+1)*gap + ii*vpw, gap, vpw, vph);
        buffer_unit_.draw(2, 0);

        text_renderer_.schedule_for_drawing(props.sampler_name,
                                            "arial"_h,
                                            (ii+1)*gap + ii*vpw,
                                            vph,
                                            1.0f,
                                            is_depth?vec3(1,0,0):vec3(1,1,1));
    }
}

void DebugOverlayRenderer::render(Scene* pscene)
{
    if(!active_) return;

    vertex_array_.bind();
    passthrough_shader_.use();
    render_pane(mode_, pscene);
    passthrough_shader_.unuse();
    vertex_array_.unbind();
}

#ifndef __DISABLE_EDITOR__
static int current_pane = 0;
static int current_tex = 0;

void DebugOverlayRenderer::generate_widget()
{
    if(!ImGui::Begin("Framebuffer peek"))
    {
        ImGui::End();
        return;
    }
    if(ImGui::SliderInt("Panel", &current_pane, 0, debug_panes_.size()-1))
    {
        current_tex = 0;
    }
    int ntex = debug_panes_[current_pane].size();
    ImGui::SliderInt("Texture", &current_tex, 0, ntex-1);
    dbg::DebugTextureProperties& props = debug_panes_[current_pane][current_tex];
    ImGui::Text("name: %s", props.sampler_name.c_str());

    ImGui::GetWindowDrawList()->AddImage((void*)(uint64_t)props.texture_index,
                                         ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
                                         ImVec2(0, 1), ImVec2(1, 0));

    ImGui::End();
}
#endif

}
