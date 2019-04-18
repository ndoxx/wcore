#ifndef SSR_RENDERER_H
#define SSR_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

class SSRRenderer : public Renderer<Vertex3P>
{
public:
    SSRRenderer();
    virtual ~SSRRenderer();

    virtual void render(Scene* pscene) override;

    inline void toggle()                { enabled_ = !enabled_; }
    inline void set_enabled(bool value) { enabled_ = value; }
    inline bool is_enabled() const      { return enabled_; }
    inline bool& get_enabled()          { return enabled_; }

    inline float& get_hit_threshold()      { return hit_threshold_; }
    inline float& get_ray_step()           { return ray_step_; }
    inline float& get_reflection_falloff() { return reflection_falloff_; }
    inline int& get_ray_steps()            { return ray_steps_; }
    inline int& get_bin_steps()            { return bin_steps_; }

private:
    Shader SSR_shader_;
    bool enabled_;
    float hit_threshold_;
    float ray_step_;
    float reflection_falloff_;
    int ray_steps_;
    int bin_steps_;
};

} // namespace wcore

#endif
