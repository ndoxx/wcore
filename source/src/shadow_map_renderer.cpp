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

uint32_t ShadowMapRenderer::SHADOW_WIDTH  = 1024;
uint32_t ShadowMapRenderer::SHADOW_HEIGHT = 1024;

vec2 ShadowMapRenderer::SHADOW_TEXEL_SIZE(1.0f/ShadowMapRenderer::SHADOW_WIDTH, 1.0f/ShadowMapRenderer::SHADOW_HEIGHT);

ShadowMapRenderer::ShadowMapRenderer():
Renderer<Vertex3P>(),
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    sm_shader_(ShaderResource("vsm.vert;vsm.frag")),
    #ifdef __EXPERIMENTAL_VSM_BLUR__
        ping_pong_(ShaderResource("blurpass.vert;blurpass.frag"),
                   SHADOW_WIDTH/2,
                   SHADOW_HEIGHT/2),
    #endif
#else
    sm_shader_(ShaderResource("shadowmap.vert;null.frag")),
#endif
sbuffer_(nullptr)
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

math::mat4 ShadowMapRenderer::render_directional_shadow_map(Scene* pscene, float normal_offset)
{
    auto plcam = pscene->get_light_camera();
/*#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    // TODO Find a way to cache scene sorting
    pscene->sort_models_light();
#endif*/

    // Get camera matrices
    const math::mat4& Vl = plcam->get_view_matrix();       // Camera View matrix
    const math::mat4& Pl = plcam->get_projection_matrix(); // Camera Projection matrix
    math::mat4 PVl(Pl*Vl);

    GFX::disable_blending();
    sm_shader_.use();
    sbuffer_->bind_as_target();
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    GFX::clear_color();
#else
    GFX::enable_depth_testing();
    GFX::unlock_depth_buffer();
    GFX::clear_depth();
#endif
    //sm_shader_.send_uniform(H_("lt.v3_lightPosition"), pscene->get_directional_light().lock()->get_position());
    sm_shader_.send_uniform(H_("f_normalOffset"), normal_offset);
    pscene->draw_models([&](std::shared_ptr<Model> pmodel)
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
        //math::mat4 MV = Vl*M;
        math::mat4 MVP = PVl*M;
        sm_shader_.send_uniform(H_("m4_ModelViewProjection"), MVP);
        //sm_shader_.send_uniform(H_("m3_Normal"), MV.submatrix(3,3));
    }/*,
    wcore::DEFAULT_MODEL_EVALUATOR,
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    wcore::ORDER::BACK_TO_FRONT
#else
    wcore::ORDER::FRONT_TO_BACK
#endif*/
    );

    sbuffer_->unbind_as_target();
    sm_shader_.unuse();
/*
#ifdef __EXPERIMENTAL_VSM_BLUR__
    // Blur pass on shadow map
    GFX::disable_face_culling();
    //sbuffer_->generate_mipmaps(0, 0, 1);
    vertex_array_.bind();
    ping_pong_.run(*static_cast<BufferModule*>(sbuffer_),
                   BlurPassPolicy(1, SHADOW_WIDTH/2, SHADOW_HEIGHT/2),
                   [&]()
                   {
                        GFX::clear_color();
                        buffer_unit_.draw(2, 0);
                   });
#endif //__EXPERIMENTAL_VSM_BLUR__
*/
    // Restore state
    GFX::lock_depth_buffer();
    GFX::disable_depth_testing();
    GFX::disable_face_culling();

    return PVl;
}

}
