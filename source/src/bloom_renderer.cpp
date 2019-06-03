#include "bloom_renderer.h"
#include "gfx_api.h"
#include "texture.h"
#include "mesh_factory.h"
#include "logger.h"
#include "algorithms.h"
#include "math3d.h"
#include "globals.h"
#include "geometry_common.h"

namespace wcore
{

using namespace math;

BloomRenderer::BloomRenderer():
blur_pass_shader_(ShaderResource("blurpass.vert;blurpass.frag")),
kernel_(9,1.8f)
{
    for(int ii=0; ii<3; ++ii)
    {
        blur_stages_.push_back(std::make_unique<BufferModule>
        (
            "dontcare",
            std::make_unique<Texture>
            (
                std::initializer_list<TextureUnitInfo>
                {
                    TextureUnitInfo("brightTex"_h, TextureFilter(TextureFilter::MIN_LINEAR | TextureFilter::MAG_LINEAR), GL_RGBA, GL_RGBA),
                },
#ifdef __OPTIM_BLOOM_USE_PP2__
                math::pp2(GLB.WIN_W/pow(2,ii+1)),
                math::pp2(GLB.WIN_H/pow(2,ii+1)),
#else
                GLB.WIN_W/pow(2,ii+1),
                GLB.WIN_H/pow(2,ii+1),
#endif // __OPTIM_BLOOM_USE_PP2__
                TextureWrap::CLAMP_TO_EDGE
            ),
            std::vector<GLenum>({GL_COLOR_ATTACHMENT0})
        ));
    }

    GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "bloombuffer",
        std::make_unique<Texture>
        (
            std::initializer_list<TextureUnitInfo>
            {
                TextureUnitInfo("bloomTex"_h, TextureFilter(TextureFilter::MIN_LINEAR | TextureFilter::MAG_LINEAR), GL_RGBA, GL_RGBA),
            },
            GLB.WIN_W/2,
            GLB.WIN_H/2,
            TextureWrap::CLAMP_TO_EDGE
        ),
        std::vector<GLenum>({GL_COLOR_ATTACHMENT0})
    ));
}

BloomRenderer::~BloomRenderer()
{

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

    for(auto&& blur_stage: blur_stages_)
    {
        blur_stage->bind_as_target();
        Gfx::clear(CLEAR_COLOR_FLAG);
        CGEOM.draw("quad"_h);
    }

    // VERTICAL BLUR PASS
    blur_pass_shader_.send_uniform("horizontal"_h, false);

    // Blend with previous blur level
    Gfx::set_std_blending();

    bloom_buffer.bind_as_target();
    for(int ii=0; ii<blur_stages_.size(); ++ii)
    {
        if(ii==0)
            Gfx::clear(CLEAR_COLOR_FLAG);

        // Bind horizontal blur pass texture to texture unit 0
        blur_stages_[ii]->get_texture().bind(0, 0);
        float width  = blur_stages_[ii]->get_texture().get_width();
        float height = blur_stages_[ii]->get_texture().get_height();
        blur_pass_shader_.send_uniform("f_alpha"_h, bloom_alpha(ii, blur_stages_.size()));
        blur_pass_shader_.send_uniform("v2_texelSize"_h, vec2(1.f/width, 1.f/height));
        CGEOM.draw("quad"_h);
    }
    bloom_buffer.unbind_as_target();

    Gfx::disable_blending();

    blur_pass_shader_.unuse();
}

}
