#ifndef PING_PONG_BUFFER_H
#define PING_PONG_BUFFER_H

#include "shader.h"
#include "texture.h"
#include "frame_buffer.h"
#include "buffer_module.h"
#include "gaussian.h"

namespace wcore
{

class BlurPassPolicy
{
public:
    BlurPassPolicy(uint32_t n_pass,
                   uint32_t target_width,
                   uint32_t target_height,
                   float sigma = 1.0f,
                   float gamma_r = 1.0f):
    n_pass_(n_pass),
    target_width_(target_width),
    target_height_(target_height),
    gamma_r_(gamma_r),
    kernel_(7,sigma)
    {

    }

    void update(Shader& shader, bool pass_direction);

    int n_pass_;
    uint32_t target_width_;
    uint32_t target_height_;
    float gamma_r_;
    math::GaussianKernel kernel_;
};

class PingPongBuffer
{
private:
    Shader shader_;
    std::unique_ptr<Texture> texture_;
    FrameBuffer fbo_;

public:
    PingPongBuffer(const ShaderResource& shader_res,
                   std::unique_ptr<Texture> texture);

    template <typename PassPolicyT>
    void run(BufferModule& inout_buffer,
             PassPolicyT&& policy,
             std::function<void(void)> draw_func,
             uint32_t inout_unit = 0,
             uint32_t inout_index = 0)
    {
        shader_.use();
        for(uint32_t ii=0; ii< policy.n_pass_; ++ii)
        {
            // Ping
            inout_buffer.bind_as_source(inout_unit, inout_index);
            fbo_.bind_as_render_target();
            policy.update(shader_, true);
            draw_func();
            fbo_.unbind();
            inout_buffer.unbind_as_source();

            // Pong
            texture_->bind_all();
            inout_buffer.bind_as_target();
            policy.update(shader_, false);
            draw_func();
            inout_buffer.unbind_as_target();
        }
        shader_.unuse();
    }
};


}

#endif // PING_PONG_BUFFER_H
