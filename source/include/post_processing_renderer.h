#ifndef POST_PROCESSING_RENDERER_H
#define POST_PROCESSING_RENDERER_H

#include "renderer.h"
#include "shader.h"

namespace wcore
{

class PostProcessingRenderer : public Renderer
{
private:
    Shader post_processing_shader_;

    bool fog_enabled_;
    bool bloom_enabled_;
    bool fxaa_enabled_;
    bool dithering_enabled_;

    int acc_daltonize_mode_;
    int acc_blindness_type_;
    float fog_density_;
    float vibrance_;
    float saturation_;
    float exposure_;
    float contrast_;
    float vignette_falloff_;
    float vignette_balance_;
    float aberration_shift_;
    float aberration_strength_;
    math::vec3 gamma_;
    math::vec3 vibrance_bal_;
    math::vec3 fog_color_;

public:

    PostProcessingRenderer();
    virtual ~PostProcessingRenderer() = default;

    virtual void render(Scene* pscene) override;

    inline void set_fog_enabled(bool value)       { fog_enabled_ = value; }
    inline void set_fxaa_enabled(bool value)      { fxaa_enabled_ = value; }
    inline void set_bloom_enabled(bool value)     { bloom_enabled_ = value; }
    inline void set_dithering_enabled(bool value) { dithering_enabled_ = value; }

    inline void toggle_fog()       { fog_enabled_ = !fog_enabled_; }
    inline void toggle_fxaa()      { fxaa_enabled_ = !fxaa_enabled_; }
    inline void toggle_bloom()     { bloom_enabled_ = !bloom_enabled_; }
    inline void toggle_dithering() { dithering_enabled_ = !dithering_enabled_; }

    inline bool& get_fog_enabled_nc()   { return fog_enabled_; }
    inline bool& get_fxaa_enabled_nc()  { return fxaa_enabled_; }
    inline bool& get_bloom_enabled_nc() { return bloom_enabled_; }
    inline bool& get_dithering_enabled_nc() { return dithering_enabled_; }

    inline void set_gamma(const math::vec3& value)     { gamma_ = value; }
    inline void set_fog_color(const math::vec3& value) { fog_color_ = value; }
    inline void set_fog_density(float fog_density)     { fog_density_ = (fog_density>=0.f) ? fog_density : 0.f; }
    inline void set_saturation(float value)            { saturation_ = value; }
    inline void set_exposure(float value)              { exposure_ = value; }
    inline void set_contrast(float value)              { contrast_ = value; }
    inline void set_vibrance(float value)              { vibrance_ = value; }
    inline void set_vignette_falloff(float value)      { vignette_falloff_ = value; }
    inline void set_vignette_balance(float value)      { vignette_balance_ = value; }
    inline void set_aberration_shift(float value)      { aberration_shift_ = value; }
    inline void set_aberration_strength(float value)   { aberration_strength_ = value; }
    inline void set_acc_blindness_type(int value)      { acc_blindness_type_ = value; }
    inline void set_vibrance_balance(const math::vec3& value) { vibrance_bal_ = value; }

    inline const math::vec3& get_gamma() const     { return gamma_; }
    inline const math::vec3& get_fog_color() const { return fog_color_; }
    inline float get_fog_density() const           { return fog_density_; }
    inline float get_saturation() const            { return saturation_; }
    inline float get_exposure() const              { return exposure_; }
    inline float get_contrast() const              { return contrast_; }
    inline float get_vibrance() const              { return vibrance_; }
    inline float get_vignette_falloff() const      { return vignette_falloff_; }
    inline float get_vignette_balance() const      { return vignette_balance_; }
    inline float get_aberration_shift() const      { return aberration_shift_; }
    inline float get_aberration_strength() const   { return aberration_strength_; }
    inline int get_acc_blindness_type() const      { return acc_blindness_type_; }
    inline int get_acc_daltonize_mode() const      { return acc_daltonize_mode_; }
    inline const math::vec3& get_vibrance_balance() const { return vibrance_bal_; }

    inline math::vec3& get_gamma_nc()            { return gamma_; }
    inline math::vec3& get_fog_color_nc()        { return fog_color_; }
    inline float& get_fog_density_nc()           { return fog_density_; }
    inline float& get_saturation_nc()            { return saturation_; }
    inline float& get_exposure_nc()              { return exposure_; }
    inline float& get_contrast_nc()              { return contrast_; }
    inline float& get_vibrance_nc()              { return vibrance_; }
    inline float& get_vignette_falloff_nc()      { return vignette_falloff_; }
    inline float& get_vignette_balance_nc()      { return vignette_balance_; }
    inline float& get_aberration_shift_nc()      { return aberration_shift_; }
    inline float& get_aberration_strength_nc()   { return aberration_strength_; }
    inline int& get_acc_blindness_type_nc()      { return acc_blindness_type_; }
    inline int& get_acc_daltonize_mode_nc()      { return acc_daltonize_mode_; }
    inline math::vec3& get_vibrance_balance_nc() { return vibrance_bal_; }

    inline void daltonize_next_mode() { acc_daltonize_mode_ = (acc_daltonize_mode_+1)%3; }
};

}

#endif // POST_PROCESSING_RENDERER_H
