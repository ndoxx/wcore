#ifndef SSAO_RENDERER_H
#define SSAO_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

namespace wcore
{

class SSAORenderer : public Renderer<Vertex3P>
{
private:
    Shader SSAO_shader_;
    math::vec2 out_size_;

    std::vector<math::vec3> ssao_kernel_;
    math::vec2 noise_scale_;
    float SSAO_radius_;
    float SSAO_bias_;
    float SSAO_intensity_;
    float SSAO_scale_;
    unsigned int noise_texture_;
    bool active_;

    static uint32_t KERNEL_SQRSIZE_;
    static uint32_t NOISE_SQRSIZE_;
    static uint32_t KERNEL_SIZE_;
    static uint32_t NOISE_SIZE_;

public:
    SSAORenderer();
    virtual ~SSAORenderer();

    virtual void render() override;

    inline void toggle() { active_ = !active_; }
    inline void set_enabled(bool value) { active_ = value; }

    void generate_random_kernel();

};

}

#endif // SSAO_RENDERER_H
