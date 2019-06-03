#ifndef SSAO_RENDERER_H
#define SSAO_RENDERER_H

#include <memory>

#include "renderer.h"
#include "shader.h"
#include "ping_pong_buffer.h"

namespace wcore
{

class Texture;
class SSAORenderer : public Renderer
{
private:
    Shader SSAO_shader_;
    PingPongBuffer ping_pong_;
    math::vec2 out_size_;

    std::vector<math::vec3> ssao_kernel_;
    math::vec2 noise_scale_;

    //unsigned int noise_texture_;
    std::shared_ptr<Texture> noise_texture_;

    float SSAO_radius_;
    float SSAO_bias_;
    float SSAO_vbias_;
    float SSAO_intensity_;
    float SSAO_scale_;
    BlurPassPolicy blur_policy_;

    static uint32_t KERNEL_SQRSIZE_;
    static uint32_t NOISE_SQRSIZE_;
    static uint32_t KERNEL_SIZE_;
    static uint32_t NOISE_SIZE_;

public:
    SSAORenderer();
    virtual ~SSAORenderer();

    virtual void render(Scene* pscene) override;

    inline void set_radius(float value)      { SSAO_radius_ = value; }
    inline void set_scalar_bias(float value) { SSAO_bias_ = value; }
    inline void set_vector_bias(float value) { SSAO_vbias_ = value; }
    inline void set_intensity(float value)   { SSAO_intensity_ = value; }
    inline void set_scale(float value)       { SSAO_scale_ = value; }
    inline void set_blur_policy(const BlurPassPolicy& value) { blur_policy_ = value; }

    inline float get_radius() const      { return SSAO_radius_; }
    inline float get_scalar_bias() const { return SSAO_bias_; }
    inline float get_vector_bias() const { return SSAO_vbias_; }
    inline float get_intensity() const   { return SSAO_intensity_; }
    inline float get_scale() const       { return SSAO_scale_; }
    inline const BlurPassPolicy& get_blur_policy() const { return blur_policy_; }

    inline float& get_radius_nc()      { return SSAO_radius_; }
    inline float& get_scalar_bias_nc() { return SSAO_bias_; }
    inline float& get_vector_bias_nc() { return SSAO_vbias_; }
    inline float& get_intensity_nc()   { return SSAO_intensity_; }
    inline float& get_scale_nc()       { return SSAO_scale_; }
    inline BlurPassPolicy& get_blur_policy_nc() { return blur_policy_; }

    void generate_random_kernel();
};

}

#endif // SSAO_RENDERER_H
