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

    shader.send_uniform("v2_texelSize"_h, vec2(coeff/target_width_,
                                                 coeff/target_height_));
    shader.send_uniform("horizontal"_h, pass_direction);
    if(!shader.is_variant("VARIANT_R_ONLY"_h))
        shader.send_uniform("f_alpha"_h, 1.0f);

    // send Gaussian kernel
    shader.send_uniform<int>("kernel.i_half_size"_h, kernel_.get_half_size());
    shader.send_uniform_array("kernel.f_weight[0]"_h, kernel_.data(), kernel_.get_half_size());

    if(shader.is_variant("VARIANT_COMPRESS_R"_h))
        shader.send_uniform("inv_gamma_r"_h, 1.0f/gamma_r_);
}

PingPongBuffer::PingPongBuffer(const ShaderResource& shader_res,
                               std::unique_ptr<Texture> texture):
shader_(shader_res),
texture_(std::move(texture)),
fbo_(*texture_, std::vector<GLenum>{GL_COLOR_ATTACHMENT0})
{

}

}
