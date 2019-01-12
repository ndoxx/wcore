#ifndef DEBUG_OVERLAY_RENDERER_H
#define DEBUG_OVERLAY_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

class TextRenderer;
class BufferModule;

namespace dbg
{
struct DebugTextureProperties
{
public:
    DebugTextureProperties(unsigned int texture_index,
                           const std::string& sampler_name,
                           bool is_depth):
    texture_index(texture_index),
    sampler_name(sampler_name),
    is_depth(is_depth){}

    unsigned int texture_index;
    std::string sampler_name;
    bool is_depth;
};

typedef std::vector<DebugTextureProperties> DebugPane;

}

class DebugOverlayRenderer : public Renderer<Vertex3P>
{
private:
    Shader passthrough_shader_;
    std::vector<dbg::DebugPane> debug_panes_;
    uint8_t mode_;
    bool active_;

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
    void render_pane(uint32_t index);

#ifndef __DISABLE_EDITOR__
    void generate_widget();
#endif
};

}

#endif // DEBUG_OVERLAY_RENDERER_H
