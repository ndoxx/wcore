#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <list>

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

struct Vertex3P;

struct DebugDrawRequest
{
    enum: uint8_t
    {
        SEGMENT,
        ORIGIN,
        CROSS3,
        TRIANGLE,
        QUAD,
        CUBE,
        SPHERE,
        N_TYPES
    };

    int        ttl;    // Time to live in number of frames. 0 = lives forever
    uint8_t    type;   // Type of object to render
    math::vec4 color;  // RGB color of lines
    math::mat4 model_matrix;
};

class DebugRenderer : public Renderer<Vertex3P>
{
private:
    Shader line_shader_;
    bool display_line_models_;
    float light_proxy_scale_;

    std::list<DebugDrawRequest> draw_requests_;

public:
    int light_display_mode_; // 0=disabled, 1=mini-spheres, 2=full-scale spheres
    int bb_display_mode_;    // 0=disabled, 1=OBB, 2=AABB
    bool enable_depth_test_;
    bool show_static_octree_;

    DebugRenderer();
    virtual ~DebugRenderer() = default;

    void load_geometry();
    virtual void render(Scene* pscene) override;

    inline void toggle_line_models()             { display_line_models_ = !display_line_models_; }

    inline void next_bb_display_mode()           { bb_display_mode_ = (bb_display_mode_+1)%4; }
    inline int get_bb_display_mode() const       { return bb_display_mode_; }

    inline void next_light_display_mode()        { light_display_mode_ = (light_display_mode_+1)%3; }
    inline int get_light_display_mode() const    { return light_display_mode_; }
    inline void set_light_display_mode(int mode) { light_display_mode_ = mode%3; }

    inline void clear_draw_requests() { draw_requests_.clear(); }

    inline void set_light_proxy_scale(float value) { light_proxy_scale_ = value; }

    // Show static octree neighbors of selected object for 5s
    void show_selection_neighbors(Scene* pscene, float radius=10.f);

    void request_draw_segment(const math::vec3& world_start,
                              const math::vec3& world_end,
                              int ttl = 60,
                              const math::vec3& color = math::vec3(0,1,0));
    void request_draw_sphere(const math::vec3& world_pos,
                             float radius,
                             int ttl = 60,
                             const math::vec3& color = math::vec3(0,1,0));
    void request_draw_cross3(const math::vec3& world_pos,
                             float radius,
                             int ttl = 60,
                             const math::vec3& color = math::vec3(0,1,0));
    void request_draw_cube(const math::mat4& model_patrix,
                           int ttl = 60,
                           const math::vec3& color = math::vec3(0,1,0));
};

}
#endif // DEBUG_RENDERER_H
