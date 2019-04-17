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
    bool SSR_enabled_;
    bool shadow_enabled_;
    bool lighting_enabled_;
    bool dirlight_enabled_;

    float bright_threshold_;
    float bright_knee_;
    float shadow_slope_bias_;
    float shadow_bias_;
    float normal_offset_;

public:
    LightingRenderer(ShadowMapRenderer& smr);
    virtual ~LightingRenderer() = default;

    void load_geometry();
    virtual void render(Scene* pscene) override;

    inline void set_SSAO_enabled(bool value)              { SSAO_enabled_ = value; }
    inline void set_SSR_enabled(bool value)               { SSR_enabled_ = value; }
    inline void set_lighting_enabled(bool value)          { lighting_enabled_ = value; }
    inline void set_shadow_mapping_enabled(bool value)    { shadow_enabled_ = value; }
    inline void set_directional_light_enabled(bool value) { dirlight_enabled_ = value; }

    inline void toggle_SSAO()           { SSAO_enabled_ = !SSAO_enabled_; }
    inline void toggle_SSR()            { SSR_enabled_ = !SSR_enabled_; }
    inline void toggle_lighting()       { lighting_enabled_ = !lighting_enabled_; }
    inline void toggle_shadow_mapping() { shadow_enabled_ = !shadow_enabled_; }

    inline bool& get_SSAO_enabled_nc()     { return SSAO_enabled_; }
    inline bool& get_SSR_enabled_nc()      { return SSR_enabled_; }
    inline bool& get_shadow_enabled_nc()   { return shadow_enabled_; }
    inline bool& get_lighting_enabled_nc() { return lighting_enabled_; }

    inline void set_bright_threshold(float value)  { bright_threshold_ = value; }
    inline void set_bright_knee(float value)       { bright_knee_ = value; }
    inline void set_shadow_slope_bias(float value) { shadow_slope_bias_ = value; }
    inline void set_shadow_bias(float value)       { shadow_bias_ = value; }
    inline void set_normal_offset(float value)     { normal_offset_ = value; }

    inline float get_bright_threshold() const  { return bright_threshold_; }
    inline float get_bright_knee() const       { return bright_knee_; }
    inline float get_shadow_slope_bias() const { return shadow_slope_bias_; }
    inline float get_shadow_bias() const       { return shadow_bias_; }
    inline float get_normal_offset() const     { return normal_offset_; }

    inline float& get_bright_threshold_nc()  { return bright_threshold_; }
    inline float& get_bright_knee_nc()       { return bright_knee_; }
    inline float& get_shadow_slope_bias_nc() { return shadow_slope_bias_; }
    inline float& get_shadow_bias_nc()       { return shadow_bias_; }
    inline float& get_normal_offset_nc()     { return normal_offset_; }

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
