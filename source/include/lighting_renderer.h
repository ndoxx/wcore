#ifndef LIGHTING_RENDERER_H
#define LIGHTING_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

struct Vertex3P;
class GBuffer;
class LBuffer;
class ShadowMapRenderer;
class LightingRenderer : public Renderer<Vertex3P>
{
private:
    //Shader lighting_pass_shader_;
    Shader lpass_dir_shader_;
    Shader lpass_point_shader_;
    Shader null_shader_;

    // Geometry data
    size_t buffer_offsets_[3];
    size_t num_elements_[3];
    ShadowMapRenderer& smr_;

    bool SSAO_enabled_;
    bool shadow_enabled_;
    bool lighting_enabled_;

public:
    float bright_threshold_;
    float bright_knee_;
    float shadow_slope_bias_;
    float normal_offset_;

    LightingRenderer(ShadowMapRenderer& smr);
    virtual ~LightingRenderer() = default;

    void load_geometry();
    virtual void render() override;

    inline void toggle_SSAO() { SSAO_enabled_ = !SSAO_enabled_; }
    inline void set_SSAO_enabled(bool value) { SSAO_enabled_ = value; }
    inline void set_lighting_enabled(bool value) { lighting_enabled_ = value; }
    inline void set_shadow_mapping_enabled(bool value) { shadow_enabled_ = value; }
    inline bool& get_SSAO_enabled_flag()     { return SSAO_enabled_; }
    inline bool& get_shadow_enabled_flag()   { return shadow_enabled_; }
    inline bool& get_lighting_enabled_flag() { return lighting_enabled_; }

private:
    inline size_t QUAD_OFFSET()   { return buffer_offsets_[0]; }
    inline size_t SPHERE_OFFSET() { return buffer_offsets_[1]; }
    inline size_t CONE_OFFSET()   { return buffer_offsets_[2]; }
    inline size_t QUAD_NE()       { return num_elements_[0]; }
    inline size_t SPHERE_NE()     { return num_elements_[1]; }
    inline size_t CONE_NE()       { return num_elements_[2]; }
};

}

#endif // LIGHTING_RENDERER_H
