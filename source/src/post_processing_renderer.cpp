#include "post_processing_renderer.h"
#include "gfx_driver.h"
#include "l_buffer.h"
#include "math3d.h"
#include "texture.h"
#include "lights.h"
#include "camera.h"
#include "globals.h"

namespace wcore
{

using namespace math;

PostProcessingRenderer::PostProcessingRenderer():
Renderer<Vertex3P>(),
post_processing_shader_(ShaderResource("postprocessing.vert;postprocessing.frag")),
fog_enabled_(true),
bloom_enabled_(true),
fxaa_enabled_(true),
gamma_(1),
vibrance_bal_(1),
vibrance_(0),
saturation_(1),
exposure_(1.8f),
contrast_(1.0f),
vignette_falloff_(0.1f),
vignette_balance_(0.25f),
fog_color_(0),
fog_density_(0.05)
{
    load_geometry();
}

void PostProcessingRenderer::render()
{
    LBuffer& lbuffer = LBuffer::Instance();

    post_processing_shader_.use();
    // Texture samplers uniforms
    post_processing_shader_.send_uniform<int>(H_("screenTex"), 0);
    post_processing_shader_.send_uniform<int>(H_("bloomTex"), 1);
    post_processing_shader_.send_uniform<int>(H_("depthStencilTex"), 2);
    // Post processing uniforms
    post_processing_shader_.send_uniform(H_("rd.f_ca_shift"), aberration_shift_);
    post_processing_shader_.send_uniform(H_("rd.f_ca_strength"), aberration_strength_);
    post_processing_shader_.send_uniform(H_("rd.v3_gamma"), gamma_);
    post_processing_shader_.send_uniform(H_("rd.v3_vibrance_bal"), vibrance_bal_);
    post_processing_shader_.send_uniform(H_("rd.f_vibrance"), vibrance_);
    post_processing_shader_.send_uniform(H_("rd.f_saturation"), saturation_);
    post_processing_shader_.send_uniform(H_("rd.f_vignette_falloff"), vignette_falloff_);
    post_processing_shader_.send_uniform(H_("rd.f_vignette_bal"), vignette_balance_);
    post_processing_shader_.send_uniform(H_("rd.f_exposure"), exposure_);
    post_processing_shader_.send_uniform(H_("rd.f_contrast"), contrast_);
    post_processing_shader_.send_uniform(H_("rd.v2_frameBufSize"), vec2(GLB.SCR_W, GLB.SCR_H));
    // Fog
    post_processing_shader_.send_uniform(H_("rd.b_enableFog"), fog_enabled_);
    post_processing_shader_.send_uniform(H_("rd.f_fogDensity"), fog_density_);
    post_processing_shader_.send_uniform(H_("rd.v3_fogColor"), fog_color_);
    // Bloom
    post_processing_shader_.send_uniform(H_("rd.b_enableBloom"), bloom_enabled_);
    // FXAA
    post_processing_shader_.send_uniform(H_("rd.b_FXAA_enabled"), fxaa_enabled_);

    // Render textured quad to screen
    GFX::viewport(0,0,GLB.SCR_W,GLB.SCR_H);

    // Bind relevant textures
    auto pbloom = Texture::get_named_texture(H_("bloom")).lock();
    lbuffer.bind_as_source(0,0);
    if(bloom_enabled_)
        pbloom->bind(1,0);
    lbuffer.bind_as_source(2,2);

    // Draw triangles in screen quad
    vertex_array_.bind();
    buffer_unit_.draw(2, 0);
    lbuffer.unbind_as_source();
    if(bloom_enabled_)
        pbloom->unbind();
    post_processing_shader_.unuse();
    vertex_array_.unbind();
}

}
