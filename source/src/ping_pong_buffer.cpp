#include "ping_pong_buffer.h"
#include <iostream>
namespace wcore
{

using namespace math;

void BlurPassPolicy::update(Shader& shader, bool pass_direction)
{
    shader.send_uniform(H_("v2_texelSize"), vec2(1.0f/target_width_,
                                                 1.0f/target_height_));
    shader.send_uniform(H_("horizontal"), pass_direction);
    shader.send_uniform(H_("f_alpha"), 1.0f);

    // send Gaussian kernel
    shader.send_uniform<int>(H_("kernel.i_half_size"), kernel_.get_half_size());
    shader.send_uniform_array(H_("kernel.f_weight[0]"), kernel_.data(), kernel_.get_half_size());

    if(shader.is_variant(H_("VARIANT_COMPRESS_R")))
    {
        shader.send_uniform(H_("inv_gamma_r"), 1.0f/gamma_r_);
    }
}

PingPongBuffer::PingPongBuffer(const ShaderResource& shader_res,
                               uint32_t width,
                               uint32_t height):
shader_(shader_res),
texture_(std::make_shared<Texture>(
            std::vector<hash_t>{H_("tmp0Tex")},
            width,
            height,
            GL_TEXTURE_2D,
            GL_LINEAR,
            GL_RGBA32F,
            GL_RGBA,
            true)),
fbo_(*texture_, std::vector<GLenum>{GL_COLOR_ATTACHMENT0})
{

}

}
