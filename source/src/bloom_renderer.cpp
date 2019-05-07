#include "bloom_renderer.h"
#include "gfx_driver.h"
#include "texture.h"
#include "mesh_factory.h"
#include "logger.h"
#include "algorithms.h"
#include "math3d.h"
#include "globals.h"
#include "geometry_common.h"
#include "buffer_module.h"

namespace wcore
{

using namespace math;

BloomRenderer::BloomRenderer():
blur_pass_shader_(ShaderResource("blurpass.vert;blurpass.frag")),
kernel_(9,1.8f)
{
    for(int ii=0; ii<3; ++ii)
    {
        bloom_h_tex_.push_back(std::make_shared<Texture>(
                std::vector<hash_t>{"brightTex"_h},
#ifdef __OPTIM_BLOOM_USE_PP2__
                    math::pp2(GLB.WIN_W/pow(2,ii+1)),
                    math::pp2(GLB.WIN_H/pow(2,ii+1)),
#else
                    GLB.WIN_W/pow(2,ii+1),
                    GLB.WIN_H/pow(2,ii+1),
#endif // __OPTIM_BLOOM_USE_PP2__
                    GL_TEXTURE_2D,
                    GL_LINEAR,
                    GL_RGB,
                    GL_RGB,
                    true)); // Clamp to avoid side artifacts

        fbo_h_.push_back(new FrameBuffer(*bloom_h_tex_[ii], {GL_COLOR_ATTACHMENT0}));
    }

    GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "bloombuffer",
        std::make_unique<Texture>(
            std::vector<hash_t>{"bloomTex"_h},
            std::vector<GLenum>{GL_LINEAR},
            std::vector<GLenum>{GL_RGB},
            std::vector<GLenum>{GL_RGB},
            GLB.WIN_W/2,
            GLB.WIN_H/2,
            GL_TEXTURE_2D,
            true),
        std::vector<GLenum>({GL_COLOR_ATTACHMENT0})
    ));
}

BloomRenderer::~BloomRenderer()
{
    for(int ii=0; ii<fbo_h_.size(); ++ii)
    {
        delete fbo_h_[ii];
    }
    //delete fbo_;
}

static inline float bloom_alpha(int index, int n_channels)
{
    return 1.0f - (index+0.5f)/(1.0f*n_channels);
}

void BloomRenderer::render(Scene* pscene)
{
    auto& l_buffer     = GMODULES::GET("lbuffer"_h);
    auto& bloom_buffer = GMODULES::GET("bloombuffer"_h);

    blur_pass_shader_.use();

    // Generate mipmaps for brightmap (texture unit 1, 4 levels total)
    l_buffer.get_texture().generate_mipmaps(1, 0, 3);

    // Bind bright map texture to texture unit 0
    l_buffer.get_texture().bind(0,1);

    // HORIZONTAL BLUR PASS
    blur_pass_shader_.send_uniform("horizontal"_h, true);
    blur_pass_shader_.send_uniform("v2_texelSize"_h, vec2(2.0f/GLB.WIN_W,
                                                            2.0f/GLB.WIN_H));
    // send Gaussian kernel
    blur_pass_shader_.send_uniform<int>("kernel.i_half_size"_h, kernel_.get_half_size());
    blur_pass_shader_.send_uniform_array("kernel.f_weight[0]"_h, kernel_.data(), kernel_.get_half_size());
    for(int ii=0; ii<fbo_h_.size(); ++ii)
    {
        fbo_h_[ii]->with_render_target([&]()
        {
            GFX::clear_color();
            CGEOM.draw("quad"_h);
        });
    }

    // VERTICAL BLUR PASS
    blur_pass_shader_.send_uniform("horizontal"_h, false);
    // Blend with previous blur level
    GFX::enable_blending();
    GFX::set_std_blending();

    bloom_buffer.bind_as_target();
    for(int ii=0; ii<fbo_h_.size(); ++ii)
    {
        if(ii==0)
            GFX::clear_color();

        // Bind horizontal blur pass texture to texture unit 0
        bloom_h_tex_[ii]->bind(0, 0);
        blur_pass_shader_.send_uniform("f_alpha"_h, bloom_alpha(ii, fbo_h_.size()));
        blur_pass_shader_.send_uniform("v2_texelSize"_h, vec2(1.0f/bloom_h_tex_[ii]->get_width(),
                                                                1.0f/bloom_h_tex_[ii]->get_height()));
        CGEOM.draw("quad"_h);
    }
    bloom_buffer.unbind_as_target();

    GFX::disable_blending();

    blur_pass_shader_.unuse();
}

}
