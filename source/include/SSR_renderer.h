#ifndef SSR_RENDERER_H
#define SSR_RENDERER_H

#include "renderer.hpp"
#include "buffer_module.h"
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
    inline bool& get_blur_enabled()     { return blur_enabled_; }

    inline float& get_dither_amount()      { return dither_amount_; }
    inline int& get_ray_steps()            { return ray_steps_; }
    inline int& get_bin_steps()            { return bin_steps_; }

    inline float& get_fade_eye_start()     { return fade_eye_start_; }
    inline float& get_fade_eye_end()       { return fade_eye_end_; }
    inline float& get_fade_screen_edge()   { return fade_screen_edge_; }
    inline float& get_min_glossiness()     { return min_glossiness_; }
    inline float& get_pix_thickness()      { return pix_thickness_; }
    inline float& get_pix_stride_cuttoff() { return pix_stride_cuttoff_; }
    inline float& get_pix_stride()         { return pix_stride_; }
    inline float& get_max_ray_distance()   { return max_ray_distance_; }

private:
    Shader SSR_shader_;
    Shader SSR_blur_shader_;

    BufferModule blur_buffer_;

    bool nonce_;
    bool enabled_;
    bool blur_enabled_;
    int ray_steps_;
    int bin_steps_;
    float dither_amount_;
    float fade_eye_start_;
    float fade_eye_end_;
    float fade_screen_edge_;
    float min_glossiness_;
    float pix_thickness_;
    float pix_stride_cuttoff_;
    float pix_stride_;
    float max_ray_distance_;
};

} // namespace wcore

#endif
