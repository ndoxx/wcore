#include "ping_pong_buffer.h"

namespace wcore
{

using namespace math;

void BlurPassPolicy::update(Shader& shader, bool pass_direction)
{
    float coeff = 1.0f;
    /*if(pass_direction)
        coeff = 2.0f;*/

    shader.send_uniform(H_("v2_texOffset"), vec2(coeff/target_width_,
                                                 coeff/target_height_));
    shader.send_uniform(H_("horizontal"), pass_direction);
    shader.send_uniform(H_("f_alpha"), 1.0f);

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
