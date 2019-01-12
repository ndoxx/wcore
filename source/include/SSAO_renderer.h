#ifndef SSAO_RENDERER_H
#define SSAO_RENDERER_H

#include "renderer.hpp"
#include "shader.h"
#include "ping_pong_buffer.h"

namespace wcore
{

class SSAORenderer : public Renderer<Vertex3P>
{
private:
    Shader SSAO_shader_;
    PingPongBuffer ping_pong_;
    math::vec2 out_size_;

    std::vector<math::vec3> ssao_kernel_;
    math::vec2 noise_scale_;

    unsigned int noise_texture_;
    bool active_;

    static uint32_t KERNEL_SQRSIZE_;
    static uint32_t NOISE_SQRSIZE_;
    static uint32_t KERNEL_SIZE_;
    static uint32_t NOISE_SIZE_;

public:
    float SSAO_radius_;
    float SSAO_bias_;
    float SSAO_vbias_;
    float SSAO_intensity_;
    float SSAO_scale_;
    BlurPassPolicy blur_policy_;

    SSAORenderer();
    virtual ~SSAORenderer();

    virtual void render(Scene* pscene) override;

    inline void toggle() { active_ = !active_; }
    inline void set_enabled(bool value) { active_ = value; }
    inline bool is_active() const { return active_; }
    inline bool& get_active() { return active_; }

    void generate_random_kernel();
};

}

#endif // SSAO_RENDERER_H
