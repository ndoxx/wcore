#include "post_processing_renderer.h"
#include "gfx_api.h"
#include "math3d.h"
#include "texture.h"
#include "lights.h"
#include "camera.h"
#include "scene.h"
#include "globals.h"
#include "geometry_common.h"
#include "buffer_module.h"

namespace wcore
{

using namespace math;

PostProcessingRenderer::PostProcessingRenderer():
post_processing_shader_(ShaderResource("postprocessing.vert;postprocessing.frag")),
fog_enabled_(true),
bloom_enabled_(true),
fxaa_enabled_(true),
dithering_enabled_(false),
acc_daltonize_mode_(0),
acc_blindness_type_(0),
fog_density_(0.05),
vibrance_(0),
saturation_(1),
exposure_(1.8f),
contrast_(1.0f),
vignette_falloff_(0.1f),
vignette_balance_(0.25f),
aberration_shift_(0.0f),
aberration_strength_(0.0f),
gamma_(1.f),
vibrance_bal_(1.f),
fog_color_(0.f)
{

}

void PostProcessingRenderer::render(Scene* pscene)
{
    auto& l_buffer      = GMODULES::GET("lbuffer"_h);
    auto& bloom_buffer  = GMODULES::GET("bloombuffer"_h);

    post_processing_shader_.use();
    // Texture samplers uniforms
    post_processing_shader_.send_uniform<int>("screenTex"_h, 0);
    post_processing_shader_.send_uniform<int>("bloomTex"_h, 1);
    post_processing_shader_.send_uniform<int>("depthStencilTex"_h, 2);
    // Post processing uniforms
    post_processing_shader_.send_uniform("rd.f_ca_shift"_h, aberration_shift_);
    post_processing_shader_.send_uniform("rd.f_ca_strength"_h, aberration_strength_);
    post_processing_shader_.send_uniform("rd.v3_gamma"_h, gamma_);
    post_processing_shader_.send_uniform("rd.v3_vibrance_bal"_h, vibrance_bal_);
    post_processing_shader_.send_uniform("rd.f_vibrance"_h, vibrance_);
    post_processing_shader_.send_uniform("rd.f_saturation"_h, saturation_);
    post_processing_shader_.send_uniform("rd.f_vignette_falloff"_h, vignette_falloff_);
    post_processing_shader_.send_uniform("rd.f_vignette_bal"_h, vignette_balance_);
    post_processing_shader_.send_uniform("rd.f_exposure"_h, exposure_);
    post_processing_shader_.send_uniform("rd.f_contrast"_h, contrast_);
    post_processing_shader_.send_uniform("rd.v2_frameBufSize"_h, vec2(GLB.WIN_W, GLB.WIN_H));
    // Fog
    post_processing_shader_.send_uniform("rd.b_enableFog"_h, fog_enabled_);
    post_processing_shader_.send_uniform("rd.f_fogDensity"_h, fog_density_);
    post_processing_shader_.send_uniform("rd.v3_fogColor"_h, fog_color_);
    post_processing_shader_.send_uniform("rd.f_cam_near"_h, pscene->get_camera().get_near());
    post_processing_shader_.send_uniform("rd.f_cam_far"_h, pscene->get_camera().get_far());

    // Bloom
    post_processing_shader_.send_uniform("rd.b_enableBloom"_h, bloom_enabled_);
    // FXAA
    post_processing_shader_.send_uniform("rd.b_FXAA_enabled"_h, fxaa_enabled_);
    // Dithering
    post_processing_shader_.send_uniform("rd.b_dither"_h, dithering_enabled_);
    // Accessibility
    post_processing_shader_.send_uniform("rd.i_daltonize_mode"_h, acc_daltonize_mode_);
    post_processing_shader_.send_uniform("rd.i_blindness_type"_h, acc_blindness_type_);


    // Render textured quad to screen
    Gfx::device->viewport(0,0,GLB.WIN_W,GLB.WIN_H);
    //Gfx::device->bind_default_frame_buffer();

    // Bind relevant textures
    l_buffer.bind_as_source(0,0);
    if(bloom_enabled_)
        bloom_buffer.get_texture().bind(1,0);
    l_buffer.bind_as_source(2,2);

    // Draw triangles in screen quad
    CGEOM.draw("quad"_h);
    l_buffer.unbind_as_source();
    if(bloom_enabled_)
        bloom_buffer.get_texture().unbind();
    post_processing_shader_.unuse();
}

}
