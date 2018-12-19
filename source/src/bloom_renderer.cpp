#include "bloom_renderer.h"
#include "gfx_driver.h"
#include "texture.h"
#include "mesh_factory.h"
#include "logger.h"
#include "algorithms.h"
#include "math3d.h"
#include "globals.h"

namespace wcore
{

using namespace math;

BloomRenderer::BloomRenderer():
Renderer<Vertex3P>(),
blur_pass_shader_(ShaderResource("blurpass.vert;blurpass.frag")),
kernel_(9,1.8f)
{
    load_geometry();

    for(int ii=0; ii<3; ++ii)
    {
        bloom_h_tex_.push_back(std::make_shared<Texture>(
                std::vector<hash_t>{H_("brightTex")},
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

    auto bloom_tex = std::make_shared<Texture>(
                            std::vector<hash_t>{H_("bloomTex")},
                            GLB.WIN_W/2,
                            GLB.WIN_H/2,
                            GL_TEXTURE_2D,
                            GL_LINEAR,
                            GL_RGB,
                            GL_RGB,
                            true);
    Texture::register_named_texture(H_("bloom"), bloom_tex);
    fbo_ = new FrameBuffer(*bloom_tex, {GL_COLOR_ATTACHMENT0});
}

BloomRenderer::~BloomRenderer()
{
    for(int ii=0; ii<fbo_h_.size(); ++ii)
    {
        delete fbo_h_[ii];
    }
    delete fbo_;
}

static inline float bloom_alpha(int index, int n_channels)
{
    return 1.0f - (index+0.5f)/(1.0f*n_channels);
}

void BloomRenderer::render()
{
    // Get access to lbuffer texture (texture index 1 is "brightTex")
    auto pscreen = Texture::get_named_texture(H_("lbuffer")).lock();

    blur_pass_shader_.use();
    vertex_array_.bind();

    // Generate mipmaps for brightmap (texture unit 1, 4 levels total)
    pscreen->generate_mipmaps(1, 0, 3);

    // Bind bright map texture to texture unit 0
    pscreen->bind(0,1);

    // HORIZONTAL BLUR PASS
    blur_pass_shader_.send_uniform(H_("horizontal"), true);
    blur_pass_shader_.send_uniform(H_("v2_texelSize"), vec2(2.0f/GLB.WIN_W,
                                                            2.0f/GLB.WIN_H));
    // send Gaussian kernel
    blur_pass_shader_.send_uniform<int>(H_("kernel.i_half_size"), kernel_.get_half_size());
    blur_pass_shader_.send_uniform_array(H_("kernel.f_weight[0]"), kernel_.data(), kernel_.get_half_size());
    for(int ii=0; ii<fbo_h_.size(); ++ii)
    {
        fbo_h_[ii]->with_render_target([&]()
        {
            GFX::clear_color();
            buffer_unit_.draw(2, 0);
        });
    }

    // VERTICAL BLUR PASS
    blur_pass_shader_.send_uniform(H_("horizontal"), false);
    // Blend with previous blur level
    GFX::enable_blending();
    GFX::set_std_blending();
    fbo_->with_render_target([&]()
    {
        for(int ii=0; ii<fbo_h_.size(); ++ii)
        {
            if(ii==0)
                GFX::clear_color();

            // Bind horizontal blur pass texture to texture unit 0
            bloom_h_tex_[ii]->bind(0, 0);
            blur_pass_shader_.send_uniform(H_("f_alpha"), bloom_alpha(ii, fbo_h_.size()));
            blur_pass_shader_.send_uniform(H_("v2_texelSize"), vec2(1.0f/bloom_h_tex_[ii]->get_width(),
                                                                    1.0f/bloom_h_tex_[ii]->get_height()));

            buffer_unit_.draw(2, 0);
        }
    });
    GFX::disable_blending();

    vertex_array_.unbind();
    blur_pass_shader_.unuse();
}

}
