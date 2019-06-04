#include "gfx_api.h"
#include "shadow_map_renderer.h"
#include "config.h"
#include "geometry_renderer.h"
#include "lights.h"
#include "model.h"
#include "camera.h"
#include "scene.h"
#include "logger.h"
#include "texture.h"
#include "geometry_common.h"
#include "buffer_module.h"

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
normal_offset_(-0.013f)
{
    CONFIG.get("root.render.shadowmap.width"_h, SHADOW_WIDTH);
    CONFIG.get("root.render.shadowmap.height"_h, SHADOW_HEIGHT);
    SHADOW_TEXEL_SIZE = vec2(1.0f/SHADOW_WIDTH, 1.0f/SHADOW_HEIGHT);

    //SHADOWBUFFER.Init(SHADOW_WIDTH, SHADOW_HEIGHT);

    // Shadow map buffer
    GMODULES::REGISTER(std::make_unique<BufferModule>
    (
        "shadowmap",
        std::make_unique<Texture>
        (
            std::initializer_list<TextureUnitInfo>
            {
            #ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
                TextureUnitInfo("shadowTex"_h, TextureFilter::MIN_LINEAR, TextureIF::RGBA32F, TextureF::RGBA),
            #else
                TextureUnitInfo("shadowTex"_h, TextureFilter(TextureFilter::MAG_NEAREST | TextureFilter::MIN_NEAREST), TextureIF::DEPTH_COMPONENT24, TextureF::DEPTH_COMPONENT),
            #endif
            },
            SHADOW_WIDTH,
            SHADOW_HEIGHT,
            TextureWrap::CLAMP_TO_EDGE
        )
    ));
}

ShadowMapRenderer::~ShadowMapRenderer()
{

}

void ShadowMapRenderer::render(Scene* pscene)
{
    // TMP: For now, only render directional shadow

    // * RENDER DIRECTIONAL LIGHT SHADOW
    if(auto dir_light = pscene->get_directional_light().lock())
    {
/*#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
        // TODO Find a way to cache scene sorting
        pscene->sort_models_light();
#endif*/

        auto& shadow_buffer = GMODULES::GET("shadowmap"_h);

        // Light matrix is light camera view-projection matrix
        light_matrix_ = pscene->get_light_camera().get_view_projection_matrix();

        Gfx::disable_blending();
        sm_shader_.use();
        shadow_buffer.bind_as_target();
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
        Gfx::clear(CLEAR_COLOR_FLAG);
#else
        Gfx::set_depth_test_enabled(true);
        Gfx::set_depth_lock(false);
        Gfx::clear(CLEAR_DEPTH_FLAG);
#endif
        //sm_shader_.send_uniform("lt.v3_lightPosition"_h, dir_light->get_position());
        sm_shader_.send_uniform("f_normalOffset"_h, normal_offset_);
        pscene->draw_models([&](const Model& model)
        {
            uint32_t cull_face = model.shadow_cull_face();
            switch(cull_face)
            {
                case 1:
                    Gfx::set_cull_mode(CullMode::Front);
                    break;

                case 2:
                    Gfx::set_cull_mode(CullMode::Back);
                    break;

                default:
                    Gfx::set_cull_mode(CullMode::None);
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

        shadow_buffer.unbind_as_target();
        sm_shader_.unuse();
    /*
#ifdef __EXPERIMENTAL_VSM_BLUR__
        // Blur pass on shadow map
        GFX::disable_face_culling();
        //shadow_buffer.generate_mipmaps(0, 0, 1);
        vertex_array_.bind();
        ping_pong_.run(*static_cast<BufferModule*>(shadow_buffer),
                       BlurPassPolicy(1, SHADOW_WIDTH/2, SHADOW_HEIGHT/2),
                       [&]()
                       {
                            Gfx::clear(CLEAR_COLOR_FLAG);
                            CGEOM.draw("quad"_h);
                       });
#endif //__EXPERIMENTAL_VSM_BLUR__
    */
        // Restore state
        Gfx::set_depth_lock(true);
        Gfx::set_depth_test_enabled(false);
        Gfx::set_cull_mode(CullMode::None);
    }
}

}
