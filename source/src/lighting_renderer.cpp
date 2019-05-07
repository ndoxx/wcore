#include <algorithm>

#include "lighting_renderer.h"
#include "gfx_driver.h"
#include "config.h"
#include "scene.h"
#include "camera.h"
#include "shadow_map_renderer.h"
#include "lights.h"
#include "texture.h"
#include "mesh_factory.h"
#include "model.h"
#include "geometry_common.h"
#include "buffer_module.h"

namespace wcore
{

LightingRenderer::LightingRenderer():
lpass_dir_shader_(ShaderResource("lpass_exp.vert;lpass_exp.frag", "VARIANT_DIRECTIONAL")),
lpass_point_shader_(ShaderResource("lpass_exp.vert;lpass_exp.frag", "VARIANT_POINT")),
null_shader_(ShaderResource("null.vert;null.frag")),
SSAO_enabled_(true),
SSR_enabled_(true),
shadow_enabled_(true),
lighting_enabled_(true),
dirlight_enabled_(true),
bright_threshold_(1.0f),
bright_knee_(0.1f),
shadow_slope_bias_(0.1f)
{
    CONFIG.get("root.render.override.allow_shadow_mapping"_h, shadow_enabled_);
}

static const math::mat4 biasMatrix
(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 0.5, 0.5,
    0.0, 0.0, 0.0, 1.0
);

static uint32_t SHADOW_TEX = 3;
static uint32_t SSAO_TEX = 4;
static uint32_t SSR_TEX = 5;

void LightingRenderer::render(Scene* pscene)
{
    auto& ssao_buffer   = GMODULES::GET("SSAObuffer"_h);
    auto& ssr_buffer    = GMODULES::GET("SSRbuffer"_h);
    auto& l_buffer      = GMODULES::GET("lbuffer"_h);
    auto& g_buffer      = GMODULES::GET("gbuffer"_h);
    auto& shadow_buffer = GMODULES::GET("shadowmap"_h);

    // Get camera matrices
    const math::mat4& V = pscene->get_camera().get_view_matrix();       // Camera View matrix
    const math::mat4& P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    math::mat4 V_inv;
    math::inverse_affine(V, V_inv);
    math::mat4 PV(P*V);

    math::vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));

    if(lighting_enabled_)
    {
        g_buffer.bind_as_source();
        l_buffer.bind_as_target();
        g_buffer.blit_depth(l_buffer); // [OPT] Find a workaround
        GFX::lock_depth_buffer();
        GFX::clear_color();

        GFX::enable_stencil();
        GFX::set_depth_test_less();
        GFX::set_light_blending();
        GFX::set_stencil_op_light_volume();

        pscene->traverse_lights([&](const Light& light, uint32_t chunk_index)
        {
            // Get light properties
            if(light.get_geometry() == 0) return;
            math::mat4 M = light.get_model_matrix();
            // Is cam inside light volume?
            //bool inside = light.surrounds_camera(*pscene->get_camera());

    // -------- STENCIL PASS
            GFX::lock_color_buffer(); // Do not write to color buffers
            GFX::clear_stencil();
            GFX::enable_depth_testing();
            GFX::disable_face_culling();    // Disable face culling, we want to process all polygons
            GFX::set_stencil_always_pass(); // Stencil test always succeeds, we just need stencil operation

            // Use null technique (void fragment shader)
            // Only stencil operation is important
            null_shader_.use();
            null_shader_.send_uniform("m4_ModelViewProjection"_h, PV*M);
            CGEOM.draw("sphere"_h);
            null_shader_.unuse();

            // Restore writing to color buffers
            l_buffer.rebind_draw_buffers();

        // ---- LIGHT PASS
            GFX::lock_stencil();
            GFX::set_stencil_notequal(0, 0xFF);
            GFX::disable_depth_testing();
            GFX::enable_blending();
            GFX::cull_front();

            lpass_point_shader_.use();
            //lpass_point_shader_.send_uniform("rd.b_lighting_enabled"_h, lighting_enabled_);
            // view position uniform
            //lpass_point_shader_.send_uniform("rd.v3_viewPos"_h, pscene->get_camera()->get_position());
            // G-Buffer texture samplers
            lpass_point_shader_.send_uniforms(g_buffer.get_texture());
            // Bright pass threshold
            lpass_point_shader_.send_uniform("rd.f_bright_threshold"_h, std::max(bright_threshold_, bright_knee_));
            lpass_point_shader_.send_uniform("rd.f_bright_knee"_h, bright_knee_);
            // Screen size
            lpass_point_shader_.send_uniform("rd.v2_screenSize"_h,
                                             math::vec2(g_buffer.get_width(),g_buffer.get_height()));
            // Light uniforms
            lpass_point_shader_.send_uniform("lt.v3_lightPosition"_h, V*light.get_position());
            lpass_point_shader_.send_uniforms(light);
            lpass_point_shader_.send_uniform("m4_ModelViewProjection"_h, PV*M);
            //lpass_point_shader_.send_uniform("m4_ModelView"_h, V*M);
            // For position reconstruction
            lpass_point_shader_.send_uniform("rd.v4_proj_params"_h, proj_params);

            // SSAO
            if(SSAO_enabled_)
            {
                ssao_buffer.get_texture().bind(SSAO_TEX,0); // Bind ssao[0] to texture unit SSAO_TEX
                lpass_point_shader_.send_uniform<int>("SSAOTex"_h, SSAO_TEX);
            }
            lpass_point_shader_.send_uniform("rd.b_enableSSAO"_h, SSAO_enabled_);

            CGEOM.draw("sphere"_h);
            lpass_point_shader_.unuse();

            GFX::disable_blending();
            GFX::disable_face_culling();
            GFX::unlock_stencil();
        },
        [&](const Light& light)
        {
            return light.is_in_frustum(pscene->get_camera());
        });
        GFX::disable_stencil();

        // Render directional light shadow to shadow buffer
        g_buffer.unbind_as_source();
        l_buffer.unbind_as_target();
    }

    if(!dirlight_enabled_) return;
    if(auto dir_light = pscene->get_directional_light().lock())
    {
    // ---- DIRECTIONAL LIGHT PASS
        g_buffer.bind_as_source();
        l_buffer.bind_as_target();

        if(lighting_enabled_)
            GFX::enable_blending();
        else
            GFX::disable_blending();
        GFX::cull_back();

        math::mat4 Id;
        Id.init_identity();
        lpass_dir_shader_.use();
        lpass_dir_shader_.send_uniform("rd.b_lighting_enabled"_h, lighting_enabled_);
        // view position uniform
        //lpass_dir_shader_.send_uniform("rd.v3_viewPos"_h, pscene->get_camera()->get_position());
        // G-Buffer texture samplers
        lpass_dir_shader_.send_uniforms(g_buffer.get_texture());
        // For position reconstruction
        lpass_dir_shader_.send_uniform("rd.v4_proj_params"_h, proj_params);
        // Bright pass threshold
        lpass_dir_shader_.send_uniform("rd.f_bright_threshold"_h, 1.0f);
        // Screen size
        lpass_dir_shader_.send_uniform("rd.v2_screenSize"_h,
                                           math::vec2(g_buffer.get_width(),g_buffer.get_height()));
        // Light uniforms
        lpass_dir_shader_.send_uniform("lt.v3_lightPosition"_h, V.submatrix(3,3)*dir_light->get_position());
        lpass_dir_shader_.send_uniforms(*dir_light);
        lpass_dir_shader_.send_uniform("m4_ModelViewProjection"_h, Id);

        // Shadow map
        lpass_dir_shader_.send_uniform("rd.b_shadow_enabled"_h, shadow_enabled_);
        if(shadow_enabled_)
        {
            math::mat4 lightMatrix = biasMatrix*pscene->get_light_camera().get_view_projection_matrix()*V_inv;

            shadow_buffer.get_texture().bind(SHADOW_TEX,0); // Bind shadow[0] to texture unit SHADOW_TEX
            lpass_dir_shader_.send_uniform<int>("shadowTex"_h, SHADOW_TEX);
            lpass_dir_shader_.send_uniform("m4_LightSpace"_h, lightMatrix);
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
            shadow_buffer.get_texture().generate_mipmaps(0);
#else
            lpass_dir_shader_.send_uniform("rd.v2_shadowTexelSize"_h, ShadowMapRenderer::SHADOW_TEXEL_SIZE);
            lpass_dir_shader_.send_uniform("rd.f_shadowBias"_h, shadow_bias_ * ShadowMapRenderer::SHADOW_TEXEL_SIZE.x());
#endif
        }

        // SSAO
        if(SSAO_enabled_)
        {
            ssao_buffer.get_texture().bind(SSAO_TEX,0); // Bind ssao[0] to texture unit SSAO_TEX
            lpass_dir_shader_.send_uniform<int>("SSAOTex"_h, SSAO_TEX);
        }
        lpass_dir_shader_.send_uniform("rd.b_enableSSAO"_h, SSAO_enabled_);

        // SSR
        if(SSR_enabled_)
        {
            ssr_buffer.get_texture().bind(SSR_TEX,0); // Bind ssr[0] to texture unit SSR_TEX
            lpass_dir_shader_.send_uniform<int>("SSRTex"_h, SSR_TEX);
        }
        lpass_dir_shader_.send_uniform("rd.b_enableSSR"_h, SSR_enabled_);

        CGEOM.draw("quad"_h);
        lpass_dir_shader_.unuse();

        GFX::disable_blending();
        GFX::disable_face_culling();

        //glDrawBuffer(GL_FRONT_AND_BACK);
    }
}

}
