#ifndef DEBUG_OVERLAY_RENDERER_H
#define DEBUG_OVERLAY_RENDERER_H

#include "renderer.hpp"
#include "shader.h"
#include "buffer_module.h"

namespace wcore
{

class TextRenderer;
class BufferModule;

namespace dbg
{
struct DebugTextureProperties
{
public:
    DebugTextureProperties(uint32_t texture_index,
                           const std::string& sampler_name,
                           bool is_depth):
    texture_index(texture_index),
    sampler_name(sampler_name),
    is_depth(is_depth){}

    uint32_t texture_index;
    std::string sampler_name;
    bool is_depth;
};

typedef std::vector<DebugTextureProperties> DebugPane;

}

class DebugOverlayRenderer : public Renderer<Vertex3P>
{
public:
    DebugOverlayRenderer(TextRenderer& text_renderer);
    virtual ~DebugOverlayRenderer() = default;

    inline void toggle()                { active_ = !active_; }
    inline void set_enabled(bool value) { active_ = value; }
    inline bool& get_enabled_flag()     { return active_; }
    inline void next_pane() { if(active_) current_pane_ = (current_pane_+1)%debug_panes_.size(); }

    // Register a group of textures to be debugged
    void register_debug_pane(std::vector<unsigned int>&& texture_indices,
                             std::vector<std::string>&& sampler_names,
                             std::vector<bool>&& is_depth);

    // Register each texture in a buffer module in the same debug group
    void register_debug_pane(BufferModule& buffer_module);

    // Render current debug pane if active
    virtual void render(Scene* pscene) override;

    // Render a full pane of engine textures at the bottom of the screen
    void render_pane(uint32_t index, Scene* pscene);

    // Render current engine texture to internal framebuffer
    void render_internal(Scene* pscene);

#ifndef __DISABLE_EDITOR__
    void generate_widget(Scene* pscene);
#endif

protected:
    bool save_fb_to_image(const std::string& filename);

private:
    Shader peek_shader_;
    BufferModule render_target_;
    std::vector<dbg::DebugPane> debug_panes_;
    int current_pane_;
    int current_tex_;
    bool active_;
    bool raw_;
    bool tone_map_;
    bool show_r_;
    bool show_g_;
    bool show_b_;
    bool invert_color_;
    bool split_alpha_;
    float split_pos_;

    TextRenderer& text_renderer_;
};

}

#endif // DEBUG_OVERLAY_RENDERER_H
