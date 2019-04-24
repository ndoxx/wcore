#include "SSR_renderer.h"
#include "SSR_buffer.h"
#include "g_buffer.h"
#include "l_buffer.h"
#include "gfx_driver.h"
#include "scene.h"
#include "camera.h"
#include "texture.h"
#include "globals.h"
#include "logger.h"

namespace wcore
{

using namespace math;

SSRRenderer::SSRRenderer():
Renderer<Vertex3P>(),
SSR_shader_(ShaderResource("SSR.vert;SSR.frag")),
SSR_blur_shader_(ShaderResource("SSR_blur.vert;SSR_blur.frag")),
blur_buffer_("SSRBlurBuffer",
std::make_shared<Texture>(
    std::vector<hash_t>{"SSRBlurTex"_h},
    std::vector<GLenum>{GL_LINEAR},
    std::vector<GLenum>{GL_RGBA16F},
    std::vector<GLenum>{GL_RGBA},
    GLB.WIN_W/2,
    GLB.WIN_H/2,
    GL_TEXTURE_2D,
    true),
{GL_COLOR_ATTACHMENT0}),
nonce_(true),
enabled_(true),
blur_enabled_(false),
ray_steps_(20),
bin_steps_(6),
dither_amount_(0.01f),
fade_eye_start_(0.8f),
fade_eye_end_(1.f),
fade_screen_edge_(0.85f),
min_glossiness_(0.01f),
pix_thickness_(0.4f),
pix_stride_cuttoff_(30.f),
pix_stride_(7.f),
max_ray_distance_(25.f),
probe_(1.f)
{
    load_geometry();
    SSRBuffer::Init(GLB.WIN_W/2, GLB.WIN_H/2);
}

SSRRenderer::~SSRRenderer()
{
    SSRBuffer::Kill();
}

void SSRRenderer::render(Scene* pscene)
{
    if(!enabled_) return;
    // For position reconstruction
    const mat4& P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    const mat4& V = pscene->get_camera().get_view_matrix(); // Camera View matrix
    vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));
    float near = pscene->get_camera().get_near();

    mat4 invView;
    math::inverse(V, invView);

    GBuffer& gbuffer = GBuffer::Instance();
    LBuffer& lbuffer = LBuffer::Instance();
    SSRBuffer& ssrbuffer = SSRBuffer::Instance();

    ssrbuffer.bind_as_target();

    gbuffer.bind_as_source(0,0);  // normal, metallic, ao
    gbuffer.bind_as_source(1,1);  // albedo, roughness
    gbuffer.bind_as_source(2,2);  // depth
    lbuffer.bind_as_source(3,0);  // screen (last frame)

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
    //SSR_shader_.send_uniform<int>("backDepthTex"_h, 4);

    SSR_shader_.send_uniform("rd.v2_texelSize"_h, vec2(1.0f/ssrbuffer.get_width(),1.0f/ssrbuffer.get_height()));
    SSR_shader_.send_uniform("rd.v2_viewportSize"_h, vec2(ssrbuffer.get_width(),ssrbuffer.get_height()));
    SSR_shader_.send_uniform("rd.v4_proj_params"_h, proj_params);
    SSR_shader_.send_uniform("rd.m4_projection"_h, P);
    SSR_shader_.send_uniform("rd.m4_invView"_h, invView);
    SSR_shader_.send_uniform("rd.f_near"_h, near);
    SSR_shader_.send_uniform("rd.f_minGlossiness"_h, min_glossiness_);
    SSR_shader_.send_uniform("rd.f_pixelThickness"_h, pix_thickness_);
    SSR_shader_.send_uniform("rd.f_maxRayDistance"_h, max_ray_distance_);
    SSR_shader_.send_uniform("rd.f_pixelStride"_h, pix_stride_);
    SSR_shader_.send_uniform("rd.f_pixelStrideZCuttoff"_h, pix_stride_cuttoff_);
    SSR_shader_.send_uniform("rd.f_iterations"_h, float(ray_steps_));
    SSR_shader_.send_uniform("rd.f_binSearchIterations"_h, float(bin_steps_));
    SSR_shader_.send_uniform("rd.f_screenEdgeFadeStart"_h, fade_screen_edge_);
    SSR_shader_.send_uniform("rd.f_eyeFadeStart"_h, fade_eye_start_);
    SSR_shader_.send_uniform("rd.f_eyeFadeEnd"_h, fade_eye_end_);
    SSR_shader_.send_uniform("rd.f_ditherAmount"_h, dither_amount_);


    SSR_shader_.send_uniform("rd.f_probe"_h, probe_);

    GFX::clear_color();

    vertex_array_.bind();
    buffer_unit_.draw(2, 0);

    gbuffer.unbind_as_source();
    lbuffer.unbind_as_source();
    ssrbuffer.unbind_as_target();
    SSR_shader_.unuse();

    if(blur_enabled_)
    {
        float maxBlurRadius = 4.f;
        int nSamples = 10;

        blur_buffer_.bind_as_target();
        gbuffer.bind_as_source(0,0);   // normal, metallic, ao
        gbuffer.bind_as_source(1,1);   // albedo, roughness
        gbuffer.bind_as_source(2,2);   // depth
        ssrbuffer.bind_as_source(3,0); // raw SSR

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
        SSR_blur_shader_.send_uniform("rd.v2_texelOffsetScale"_h, vec2(maxBlurRadius/ssrbuffer.get_width(), 0.f));

        GFX::clear_color();
        buffer_unit_.draw(2, 0);
        gbuffer.unbind_as_source();
        ssrbuffer.unbind_as_source();
        blur_buffer_.unbind_as_target();

        // Vertical pass
        gbuffer.bind_as_source(0,0);   // normal, metallic, ao
        gbuffer.bind_as_source(1,1);   // albedo, roughness
        gbuffer.bind_as_source(2,2);   // depth
        blur_buffer_.bind_as_source(3,0); // hblur SSR
        ssrbuffer.bind_as_target();

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
        SSR_blur_shader_.send_uniform("rd.v2_texelOffsetScale"_h, vec2(0.f, maxBlurRadius/ssrbuffer.get_height()));

        GFX::enable_blending();
        GFX::set_std_blending();
        buffer_unit_.draw(2, 0);
        gbuffer.unbind_as_source();
        blur_buffer_.unbind_as_source();
        ssrbuffer.unbind_as_target();
        GFX::disable_blending();

        SSR_blur_shader_.unuse();
    }
}

} // namespace wcore
