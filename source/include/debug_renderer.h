#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

struct Vertex3P;
class LBuffer;
class DebugRenderer : public Renderer<Vertex3P>
{
private:
    Shader line_shader_;
    bool display_line_models_;
    uint8_t light_display_mode_; // 0=disabled, 1=mini-spheres, 2=full-scale spheres
    uint8_t bb_display_mode_;    // 0=disabled, 1=OBB, 2=AABB

public:
    DebugRenderer();
    virtual ~DebugRenderer() = default;

    void load_geometry();
    virtual void render() override;

    inline void toggle_line_models() { display_line_models_ = !display_line_models_; }

    inline void next_bb_display_mode() { bb_display_mode_ = (++bb_display_mode_)%3; }
    inline uint8_t get_bb_display_mode() const { return bb_display_mode_; }

    inline void next_light_display_mode() { light_display_mode_ = (++light_display_mode_)%3; }
    inline uint8_t get_light_display_mode() const { return light_display_mode_; }
};

}
#endif // DEBUG_RENDERER_H
