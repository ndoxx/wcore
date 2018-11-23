#include "shadow_map_renderer.h"
#include "gfx_driver.h"
#include "config.h"
#include "geometry_renderer.h"
#include "lights.h"
#include "model.h"
#include "camera.h"
#include "scene.h"
#include "logger.h"
#include "texture.h"
#include "shadow_buffer.h"

namespace wcore
{

using namespace math;

uint32_t ShadowMapRenderer::SHADOW_WIDTH  = 1920;
uint32_t ShadowMapRenderer::SHADOW_HEIGHT = 1920;

vec2 ShadowMapRenderer::SHADOW_TEXEL_SIZE(1.0f/ShadowMapRenderer::SHADOW_WIDTH, 1.0f/ShadowMapRenderer::SHADOW_HEIGHT);

ShadowMapRenderer::ShadowMapRenderer():
Renderer<Vertex3P>(),
sbuffer_(nullptr),
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
sm_shader_(ShaderResource("vsm.vert;vsm.frag")),
#ifdef __EXPERIMENTAL_VSM_BLUR__
blur_pass_shader_(ShaderResource("blurpass.vert;blurpass.frag")),
tmp_tex_(std::make_shared<Texture>(
            std::vector<std::string>{"tmp0Tex"},
            SHADOW_WIDTH/2,
            SHADOW_HEIGHT/2,
            GL_TEXTURE_2D,
            GL_LINEAR,
            GL_RGBA32F,
            GL_RGBA,
            true)),
tmp_fbo_(*tmp_tex_, std::vector<GLenum>{GL_COLOR_ATTACHMENT0}),
#endif
#else
sm_shader_(ShaderResource("shadowmap.vert;null.frag"))
#endif
{
    CONFIG.get(H_("root.render.shadowmap.width"), SHADOW_WIDTH);
    CONFIG.get(H_("root.render.shadowmap.height"), SHADOW_HEIGHT);
    SHADOW_TEXEL_SIZE = vec2(1.0f/SHADOW_WIDTH, 1.0f/SHADOW_HEIGHT);

    sbuffer_ = new ShadowBuffer(SHADOW_WIDTH, SHADOW_HEIGHT);

#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    load_geometry();
#endif
}

ShadowMapRenderer::~ShadowMapRenderer()
{
    delete sbuffer_;
}

math::mat4 ShadowMapRenderer::render_directional_shadow_map()
{
    auto plcam = SCENE.get_light_camera();

    // Get camera matrices
    const math::mat4& Vl = plcam->get_view_matrix();       // Camera View matrix
    const math::mat4& Pl = plcam->get_projection_matrix(); // Camera Projection matrix
    math::mat4 PVl(Pl*Vl);

    GFX::disable_blending();
    sm_shader_.use();
    sbuffer_->bind_as_target();
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    GFX::set_clear_color(vec4(1.0,1.0,0.0,0.0));
    GFX::clear_color();
    GFX::set_clear_color(vec4(0.0,0.0,0.0,0.0));
#else
    GFX::enable_depth_testing();
    GFX::unlock_depth_buffer();
    GFX::clear_depth();
#endif
    //sm_shader_.send_uniform(H_("lt.v3_lightPosition"), SCENE.get_directional_light()->get_position());
    SCENE.draw_models([&](std::shared_ptr<Model> pmodel)
    {
        uint32_t cull_face = pmodel->shadow_cull_face();
        switch(cull_face)
        {
            case 1:
                GFX::cull_front();
                break;

            case 2:
                GFX::cull_back();
                break;

            default:
                GFX::disable_face_culling();
        }

        // Get model matrix and compute products
        math::mat4 M = pmodel->get_model_matrix();
        math::mat4 MVP = PVl*M;
        sm_shader_.send_uniform(H_("m4_ModelViewProjection"), MVP);
    });

    sbuffer_->unbind_as_target();
    sm_shader_.unuse();

#ifdef __EXPERIMENTAL_VSM_BLUR__
    // Blur pass on shadow map
    GFX::disable_face_culling();
    //sbuffer_->generate_mipmaps(0, 0, 1);
    vertex_array_.bind();
    blur_pass_shader_.use();

    // Horizontal pass
    sbuffer_->bind_as_source();
    tmp_fbo_.bind_as_render_target();

    blur_pass_shader_.send_uniform(H_("v2_texOffset"), vec2(2.0f/SHADOW_WIDTH,
                                                        2.0f/SHADOW_HEIGHT));
    blur_pass_shader_.send_uniform(H_("horizontal"), true);
    blur_pass_shader_.send_uniform(H_("f_alpha"), 1.0f);

    GFX::clear_color();
    buffer_unit_.draw(2, 0);

    tmp_fbo_.unbind();
    sbuffer_->unbind_as_source();

    // Vertical pass
    tmp_tex_->bind_all();
    sbuffer_->bind_as_target();
    blur_pass_shader_.send_uniform(H_("v2_texOffset"), vec2(1.0f/SHADOW_WIDTH,
                                                        1.0f/SHADOW_HEIGHT));
    blur_pass_shader_.send_uniform(H_("horizontal"), false);
    blur_pass_shader_.send_uniform(H_("f_alpha"), 1.0f);

    GFX::clear_color();
    buffer_unit_.draw(2, 0);

    sbuffer_->unbind_as_target();

    blur_pass_shader_.unuse();
    vertex_array_.unbind();
#endif //__EXPERIMENTAL_VSM_BLUR__

    // Restore state
    GFX::lock_depth_buffer();
    GFX::disable_depth_testing();
    GFX::disable_face_culling();

    return PVl;
}

}
