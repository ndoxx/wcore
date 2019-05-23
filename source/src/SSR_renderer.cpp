#include "SSR_renderer.h"
#include "gfx_driver.h"
#include "scene.h"
#include "camera.h"
#include "texture.h"
#include "globals.h"
#include "logger.h"
#include "geometry_common.h"

namespace wcore
{

using namespace math;

SSRRenderer::SSRRenderer():
SSR_shader_(ShaderResource("SSR.vert;SSR.frag")),
SSR_blur_shader_(ShaderResource("SSR_blur.vert;SSR_blur.frag")),
blur_buffer_("SSRBlurBuffer",
std::make_unique<Texture>(
    std::initializer_list<TextureUnitInfo>
    {
        TextureUnitInfo("SSRBlurTex"_h, TextureFilter::MIN_LINEAR, GL_RGBA16F, GL_RGBA),
    },
    GLB.WIN_W/2,
    GLB.WIN_H/2,
    TextureWrap::CLAMP_TO_EDGE),
{GL_COLOR_ATTACHMENT0}),
nonce_(true),
blur_enabled_(false),
ray_steps_(20),
bin_steps_(6),
dither_amount_(0.09f),
fade_eye_start_(0.8f),
fade_eye_end_(1.f),
fade_screen_edge_(0.85f),
min_glossiness_(0.01f),
//pix_thickness_(0.0f),
pix_stride_cuttoff_(30.f),
pix_stride_(7.f),
max_ray_distance_(25.f),
probe_(1.f)
{
    old_VP_.init_identity();
}

SSRRenderer::~SSRRenderer()
{

}

void SSRRenderer::render(Scene* pscene)
{
    auto& l_buffer = GMODULES::GET("lbuffer"_h);
    auto& g_buffer = GMODULES::GET("gbuffer"_h);
    auto& ssr_buffer = GMODULES::GET("SSRbuffer"_h);
    auto& bfd_buffer = GMODULES::GET("backfaceDepthBuffer"_h);

    // For position reconstruction
    const mat4& P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    const mat4& V = pscene->get_camera().get_view_matrix(); // Camera View matrix
    vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));
    float near = pscene->get_camera().get_near();

    static mat4 UNBIAS(2, 0, 0, -1,
                       0, 2, 0, -1,
                       0, 0, 2, 1,
                       0, 0, 0, 1);
    static mat4   BIAS(0.5, 0,   0,   0.5,
                       0,   0.5, 0,   0.5,
                       0,   0,   0.5, 0.5,
                       0,   0,   0,   1);
    mat4 VP(P*V);

    mat4 reproj;
    math::inverse(VP, reproj);
    reproj = BIAS * old_VP_ * reproj * UNBIAS;

    mat4 invView;
    math::inverse(V, invView);

    ssr_buffer.bind_as_target();

    g_buffer.bind_as_source(0,0);  // normal, metallic, ao
    g_buffer.bind_as_source(1,1);  // albedo, roughness
    g_buffer.bind_as_source(2,2);  // depth
    l_buffer.bind_as_source(3,0);  // screen (last frame)
    bfd_buffer.bind_as_source(4,0); // backface depth buffer

    SSR_shader_.use();
    SSR_shader_.send_uniform<int>("normalTex"_h, 0);
    SSR_shader_.send_uniform<int>("albedoTex"_h, 1);
    SSR_shader_.send_uniform<int>("depthTex"_h, 2);
    if(!nonce_)
        SSR_shader_.send_uniform<int>("lastFrameTex"_h, 3);
    else
    {
        // Use albedo map as "last frame" for first frame
        SSR_shader_.send_uniform<int>("lastFrameTex"_h, 1);
        nonce_ = false;
    }
    SSR_shader_.send_uniform<int>("backDepthTex"_h, 4);

    SSR_shader_.send_uniform("rd.v2_texelSize"_h, vec2(1.0f/ssr_buffer.get_width(),1.0f/ssr_buffer.get_height()));
    SSR_shader_.send_uniform("rd.v2_viewportSize"_h, vec2(ssr_buffer.get_width(),ssr_buffer.get_height()));
    SSR_shader_.send_uniform("rd.v4_proj_params"_h, proj_params);
    SSR_shader_.send_uniform("rd.m4_projection"_h, P);
    SSR_shader_.send_uniform("rd.m4_invView"_h, invView);
    SSR_shader_.send_uniform("rd.m4_reproj"_h, reproj);
    SSR_shader_.send_uniform("rd.f_near"_h, near);
    SSR_shader_.send_uniform("rd.f_minGlossiness"_h, min_glossiness_);
    //SSR_shader_.send_uniform("rd.f_pixelThickness"_h, pix_thickness_);
    SSR_shader_.send_uniform("rd.f_maxRayDistance"_h, max_ray_distance_);
    SSR_shader_.send_uniform("rd.f_pixelStride"_h, pix_stride_);
    SSR_shader_.send_uniform("rd.f_pixelStrideZCuttoff"_h, 1.f/pix_stride_cuttoff_);
    SSR_shader_.send_uniform("rd.f_iterations"_h, float(ray_steps_));
    SSR_shader_.send_uniform("rd.f_binSearchIterations"_h, float(bin_steps_));
    SSR_shader_.send_uniform("rd.f_screenEdgeFadeStart"_h, fade_screen_edge_);
    SSR_shader_.send_uniform("rd.f_eyeFadeStart"_h, fade_eye_start_);
    SSR_shader_.send_uniform("rd.f_eyeFadeEnd"_h, fade_eye_end_);
    SSR_shader_.send_uniform("rd.f_ditherAmount"_h, dither_amount_);
    SSR_shader_.send_uniform("rd.f_probe"_h, probe_);

    GFX::clear_color();

    CGEOM.draw("quad"_h);

    g_buffer.unbind_as_source();
    l_buffer.unbind_as_source();
    ssr_buffer.unbind_as_target();
    SSR_shader_.unuse();

    old_VP_ = VP;


    if(blur_enabled_)
    {
        float maxBlurRadius = 4.f;
        int nSamples = 10;

        blur_buffer_.bind_as_target();
        g_buffer.bind_as_source(0,0);   // normal, metallic, ao
        g_buffer.bind_as_source(1,1);   // albedo, roughness
        g_buffer.bind_as_source(2,2);   // depth
        ssr_buffer.bind_as_source(3,0); // raw SSR

        SSR_blur_shader_.use();
        SSR_blur_shader_.send_uniform<int>("normalTex"_h, 0);
        SSR_blur_shader_.send_uniform<int>("albedoTex"_h, 1);
        SSR_blur_shader_.send_uniform<int>("depthTex"_h, 2);
        SSR_blur_shader_.send_uniform<int>("mainTex"_h, 3);

        SSR_blur_shader_.send_uniform("rd.v2_texelSize"_h, vec2(1.0f/blur_buffer_.get_width(),1.0f/blur_buffer_.get_height()));
        SSR_blur_shader_.send_uniform("rd.v4_proj_params"_h, proj_params);
        SSR_blur_shader_.send_uniform("rd.f_depthBias"_h, 0.305f);
        SSR_blur_shader_.send_uniform("rd.f_normalBias"_h, 0.29f);
        SSR_blur_shader_.send_uniform("rd.f_blurQuality"_h, 2.f);
        SSR_blur_shader_.send_uniform<int>("rd.i_samples"_h, nSamples);

        // Horizontal pass
        SSR_blur_shader_.send_uniform("rd.v2_texelOffsetScale"_h, vec2(maxBlurRadius/ssr_buffer.get_width(), 0.f));

        GFX::clear_color();

        CGEOM.draw("quad"_h);
        g_buffer.unbind_as_source();
        ssr_buffer.unbind_as_source();
        blur_buffer_.unbind_as_target();

        // Vertical pass
        g_buffer.bind_as_source(0,0);   // normal, metallic, ao
        g_buffer.bind_as_source(1,1);   // albedo, roughness
        g_buffer.bind_as_source(2,2);   // depth
        blur_buffer_.bind_as_source(3,0); // hblur SSR
        ssr_buffer.bind_as_target();

        SSR_blur_shader_.send_uniform<int>("normalTex"_h, 0);
        SSR_blur_shader_.send_uniform<int>("albedoTex"_h, 1);
        SSR_blur_shader_.send_uniform<int>("depthTex"_h, 2);
        SSR_blur_shader_.send_uniform<int>("mainTex"_h, 3);

        SSR_blur_shader_.send_uniform("rd.v2_texelSize"_h, vec2(1.0f/blur_buffer_.get_width(),1.0f/blur_buffer_.get_height()));
        SSR_blur_shader_.send_uniform("rd.v4_proj_params"_h, proj_params);
        SSR_blur_shader_.send_uniform("rd.f_depthBias"_h, 0.305f);
        SSR_blur_shader_.send_uniform("rd.f_normalBias"_h, 0.29f);
        SSR_blur_shader_.send_uniform("rd.f_blurQuality"_h, 2.f);
        SSR_blur_shader_.send_uniform<int>("rd.i_samples"_h, nSamples);
        SSR_blur_shader_.send_uniform("rd.v2_texelOffsetScale"_h, vec2(0.f, maxBlurRadius/ssr_buffer.get_height()));

        GFX::enable_blending();
        GFX::set_std_blending();
        CGEOM.draw("quad"_h);
        g_buffer.unbind_as_source();
        blur_buffer_.unbind_as_source();
        ssr_buffer.unbind_as_target();
        GFX::disable_blending();

        SSR_blur_shader_.unuse();
    }
}

} // namespace wcore
