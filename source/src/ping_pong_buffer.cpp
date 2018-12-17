#include "ping_pong_buffer.h"
#include <iostream>
namespace wcore
{

using namespace math;

void BlurPassPolicy::update(Shader& shader, bool pass_direction)
{
    float coeff = 1.0f;
    if(pass_direction)
        coeff = 2.0f;

    shader.send_uniform(H_("v2_texelSize"), vec2(coeff/target_width_,
                                                 coeff/target_height_));
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
                               std::unique_ptr<Texture> texture):
shader_(shader_res),
texture_(std::move(texture)),
fbo_(*texture_, std::vector<GLenum>{GL_COLOR_ATTACHMENT0})
{

}

}
