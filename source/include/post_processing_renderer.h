#ifndef POST_PROCESSING_RENDERER_H
#define POST_PROCESSING_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

class PostProcessingRenderer : public Renderer<Vertex3P>
{
private:
    Shader post_processing_shader_;

public:
    bool fog_enabled_;
    bool bloom_enabled_;
    bool fxaa_enabled_;
    bool dithering_enabled_;

    math::vec3 gamma_;
    math::vec3 vibrance_bal_;
    float vibrance_;
    float saturation_;
    float exposure_;
    float contrast_;
    float vignette_falloff_;
    float vignette_balance_;
    float aberration_shift_;
    float aberration_strength_;
    int acc_daltonize_mode_;
    int acc_blindness_type_;

    math::vec3 fog_color_;
    float fog_density_;

    PostProcessingRenderer();
    virtual ~PostProcessingRenderer() = default;

    virtual void render(Scene* pscene) override;

    inline void set_fog_enabled(bool value)   { fog_enabled_ = value; }
    inline void toggle_fog()                  { fog_enabled_ = !fog_enabled_; }
    inline void set_bloom_enabled(bool value) { bloom_enabled_ = value; }
    inline bool& get_fog_enabled_flag()       { return fog_enabled_; }

    inline void toggle_fxaa()                { fxaa_enabled_ = !fxaa_enabled_; }
    inline void set_fxaa_enabled(bool value) { fxaa_enabled_ = value; }
    inline bool& get_fxaa_enabled_flag()     { return fxaa_enabled_; }

    inline void set_gamma(const math::vec3& gamma) { gamma_ = gamma; }
    inline const math::vec3& get_gamma()           { return gamma_; }
    inline math::vec3& get_gamma_nc()              { return gamma_; }

    inline void set_saturation(float saturation) { saturation_ = saturation; }
    inline float get_saturation() const          { return saturation_; }
    inline float& get_saturation_nc()            { return saturation_; }

    inline void set_fog_color(const math::vec3& fog_color) { fog_color_ = fog_color; }
    inline const math::vec3& get_fog_color()               { return fog_color_; }
    inline math::vec3& get_fog_color_nc()                  { return fog_color_; }

    inline void set_exposure(float value) { exposure_ = value; }
    inline float get_exposure() const     { return exposure_; }
    inline float& get_exposure_nc()       { return exposure_; }

    inline void set_fog_density(float fog_density)
    {
        fog_density_ = fog_density;
        if(fog_density_<=0.0f)
            fog_density_ = 0.0f;
    }
    inline float get_fog_density() const { return fog_density_; }
    inline float& get_fog_density_nc()   { return fog_density_; }

    inline void daltonize_next_mode() { acc_daltonize_mode_ = (++acc_daltonize_mode_)%3; }
};

}

#endif // POST_PROCESSING_RENDERER_H
