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
private:
    Shader passthrough_shader_;
    BufferModule render_target_;
    std::vector<dbg::DebugPane> debug_panes_;
    uint8_t mode_;
    bool active_;
    bool tone_map_;
    bool show_r_;
    bool show_g_;
    bool show_b_;
    bool split_alpha_;
    float split_pos_;

    TextRenderer& text_renderer_;

public:
    DebugOverlayRenderer(TextRenderer& text_renderer);
    virtual ~DebugOverlayRenderer() = default;

    inline void toggle()                { active_ = !active_; }
    inline void set_enabled(bool value) { active_ = value; }
    inline bool& get_enabled_flag()     { return active_; }
    inline void next_mode() { if(active_) mode_ = (mode_+1)%debug_panes_.size(); }

    virtual void render(Scene* pscene) override;

    void register_debug_pane(std::vector<unsigned int>&& texture_indices,
                             std::vector<std::string>&& sampler_names,
                             std::vector<bool>&& is_depth);
    void register_debug_pane(BufferModule& buffer_module);
    void render_pane(uint32_t index, Scene* pscene);

#ifndef __DISABLE_EDITOR__
    void generate_widget(Scene* pscene);
#endif
};

}

#endif // DEBUG_OVERLAY_RENDERER_H
