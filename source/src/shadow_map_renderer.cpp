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
#include "geometry_common.h"

namespace wcore
{

using namespace math;

uint32_t ShadowMapRenderer::SHADOW_WIDTH  = 1024;
uint32_t ShadowMapRenderer::SHADOW_HEIGHT = 1024;

vec2 ShadowMapRenderer::SHADOW_TEXEL_SIZE(1.0f/ShadowMapRenderer::SHADOW_WIDTH, 1.0f/ShadowMapRenderer::SHADOW_HEIGHT);

ShadowMapRenderer::ShadowMapRenderer():
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
sbuffer_(nullptr),
enabled_(true),
normal_offset_(-0.013f)
{
    CONFIG.get("root.render.shadowmap.width"_h, SHADOW_WIDTH);
    CONFIG.get("root.render.shadowmap.height"_h, SHADOW_HEIGHT);
    SHADOW_TEXEL_SIZE = vec2(1.0f/SHADOW_WIDTH, 1.0f/SHADOW_HEIGHT);

    sbuffer_ = new ShadowBuffer(SHADOW_WIDTH, SHADOW_HEIGHT);
}

ShadowMapRenderer::~ShadowMapRenderer()
{
    delete sbuffer_;
}

void ShadowMapRenderer::render(Scene* pscene)
{
    if(!enabled_) return;

    // TMP: For now, only render directional shadow

    // * RENDER DIRECTIONAL LIGHT SHADOW
    if(auto dir_light = pscene->get_directional_light().lock())
    {
/*#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
        // TODO Find a way to cache scene sorting
        pscene->sort_models_light();
#endif*/

        // Light matrix is light camera view-projection matrix
        light_matrix_ = pscene->get_light_camera().get_view_projection_matrix();

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
        //sm_shader_.send_uniform("lt.v3_lightPosition"_h, dir_light->get_position());
        sm_shader_.send_uniform("f_normalOffset"_h, normal_offset_);
        pscene->draw_models([&](const Model& model)
        {
            uint32_t cull_face = model.shadow_cull_face();
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
            math::mat4 M = const_cast<Model&>(model).get_model_matrix();
            //math::mat4 MV = Vl*M;
            math::mat4 MVP = light_matrix_*M;
            sm_shader_.send_uniform("m4_ModelViewProjection"_h, MVP);
            //sm_shader_.send_uniform("m3_Normal"_h, MV.submatrix(3,3));
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
                            CGEOM.draw("quad"_h);
                       });
#endif //__EXPERIMENTAL_VSM_BLUR__
    */
        // Restore state
        GFX::lock_depth_buffer();
        GFX::disable_depth_testing();
        GFX::disable_face_culling();
    }
}

}
