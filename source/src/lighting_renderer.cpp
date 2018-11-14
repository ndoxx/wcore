#include "lighting_renderer.h"
#include "gfx_driver.h"
#include "config.h"
#include "scene.h"
#include "camera.h"
#include "g_buffer.h"
#include "l_buffer.h"
#include "SSAO_buffer.h"
#include "shadow_map_renderer.h"
#include "lights.h"
#include "texture.h"
#include "mesh_factory.h"
#include "model.h"

LightingRenderer::LightingRenderer(ShadowMapRenderer& smr):
Renderer<Vertex3P>(),
lpass_dir_shader_(ShaderResource("lpass_exp.vert;lpass_exp.frag", "VARIANT_DIRECTIONAL")),
lpass_point_shader_(ShaderResource("lpass_exp.vert;lpass_exp.frag", "VARIANT_POINT")),
null_shader_(ShaderResource("null.vert;null.frag")),
smr_(smr),
SSAO_enabled_(false),
shadow_enabled_(true),
lighting_enabled_(true)
{
    CONFIG.get(H_("root.render.override.allow_shadow_mapping"), shadow_enabled_);
    load_geometry();
}

void LightingRenderer::load_geometry()
{
    Mesh<Vertex3P>* quad_mesh   = factory::make_quad_3P();
    Mesh<Vertex3P>* sphere_mesh = factory::make_uv_sphere_3P(4, 7); // factory::make_icosahedron_3P();
    buffer_unit_.submit(*quad_mesh);
    buffer_unit_.submit(*sphere_mesh);
    buffer_unit_.upload();

    buffer_offsets_[0] = quad_mesh->get_buffer_offset();
    buffer_offsets_[1] = sphere_mesh->get_buffer_offset();
    buffer_offsets_[2] = 0;

    num_elements_[0] = quad_mesh->get_n_elements();
    num_elements_[1] = sphere_mesh->get_n_elements();
    num_elements_[2] = 0;
/*
    for(int ii=0; ii<3; ++ii)
        std::cout << buffer_offsets_[ii] << " " << num_elements_[ii] << std::endl;
*/
    delete quad_mesh;
    delete sphere_mesh;
}

static const math::mat4 biasMatrix
(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 0.5, 0.5,
    0.0, 0.0, 0.0, 1.0
);

#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
        static uint32_t SHADOW_TEX = 3;
        static uint32_t SSAO_TEX = 4;
#else
        static uint32_t SHADOW_TEX = 4;
        static uint32_t SSAO_TEX = 5;
#endif

void LightingRenderer::render()
{
    GBuffer& gbuffer = GBuffer::Instance();
    LBuffer& lbuffer = LBuffer::Instance();

    // Get camera matrices
    const math::mat4& V = SCENE.get_camera()->get_view_matrix();       // Camera View matrix
    const math::mat4& P = SCENE.get_camera()->get_projection_matrix(); // Camera Projection matrix
    math::mat4 V_inv;
    math::inverse_affine(V, V_inv);
    math::mat4 PV(P*V);

    math::vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));

    auto pgbuffer = Texture::get_named_texture(H_("gbuffer")).lock();
    auto pshadow  = Texture::get_named_texture(H_("shadowmap")).lock();

    if(lighting_enabled_)
    {
        vertex_array_.bind();
        gbuffer.bind_as_source();
        lbuffer.bind_as_target();
        gbuffer.blit_depth(lbuffer); // [OPT] Find a workaround
        GFX::lock_depth_buffer();
        GFX::clear_color();

        GFX::enable_stencil();
        GFX::set_depth_test_less();
        GFX::set_light_blending();
        GFX::set_stencil_op_light_volume();

        SCENE.traverse_lights([&](std::shared_ptr<const Light> plight, uint32_t chunk_index)
        {
            // Get light properties
            if(plight->get_geometry() == 0) return;
            math::mat4 M = plight->get_model_matrix();
            // Is cam inside light volume?
            //bool inside = plight->surrounds_camera(*SCENE.get_camera());

    // -------- STENCIL PASS
            GFX::lock_color_buffer(); // Do not write to color buffers
            GFX::clear_stencil();
            GFX::enable_depth_testing();
            GFX::disable_face_culling();    // Disable face culling, we want to process all polygons
            GFX::set_stencil_always_pass(); // Stencil test always succeeds, we just need stencil operation

            // Use null technique (void fragment shader)
            // Only stencil operation is important
            null_shader_.use();
            null_shader_.send_uniform(H_("m4_ModelViewProjection"), PV*M);
            buffer_unit_.draw(SPHERE_NE(), SPHERE_OFFSET());
            null_shader_.unuse();

            // Restore writing to color buffers
            lbuffer.rebind_draw_buffers();

        // ---- LIGHT PASS
            GFX::lock_stencil();
            GFX::set_stencil_notequal(0, 0xFF);
            GFX::disable_depth_testing();
            GFX::enable_blending();
            GFX::cull_front();

            lpass_point_shader_.use();
            //lpass_point_shader_.send_uniform(H_("rd.b_lighting_enabled"), lighting_enabled_);
            // view position uniform
            //lpass_point_shader_.send_uniform(H_("rd.v3_viewPos"), SCENE.get_camera()->get_position());
            // G-Buffer texture samplers
            lpass_point_shader_.send_uniforms(*pgbuffer);
            // Bright pass threshold
            lpass_point_shader_.send_uniform(H_("rd.f_bright_threshold"), 1.0f);
            // Screen size
            lpass_point_shader_.send_uniform(H_("rd.v2_screenSize"),
                                               math::vec2(gbuffer.get_width(),gbuffer.get_height()));
            // Light uniforms
            lpass_point_shader_.send_uniform(H_("lt.v3_lightPosition"), V*plight->get_position());
            lpass_point_shader_.send_uniforms(plight);
            lpass_point_shader_.send_uniform(H_("m4_ModelViewProjection"), PV*M);
            //lpass_point_shader_.send_uniform(H_("m4_ModelView"), V*M);
#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
            // For position reconstruction
            lpass_point_shader_.send_uniform(H_("rd.v4_proj_params"), proj_params);
#endif

            // SSAO
            if(SSAO_enabled_)
            {
                auto pssao = Texture::get_named_texture(H_("SSAObuffer")).lock();
                pssao->bind(SSAO_TEX,0); // Bind ssao[0] to texture unit SSAO_TEX
                lpass_point_shader_.send_uniform<int>(H_("SSAOTex"), SSAO_TEX);
            }
            lpass_point_shader_.send_uniform(H_("rd.b_enableSSAO"), SSAO_enabled_);

            buffer_unit_.draw(SPHERE_NE(), SPHERE_OFFSET());
            lpass_point_shader_.unuse();

            GFX::disable_blending();
            GFX::disable_face_culling();
            GFX::unlock_stencil();
        },
        [&](std::shared_ptr<Light> plight)
        {
            return plight->is_in_frustum(*SCENE.get_camera());
        });
        GFX::disable_stencil();

        // Render directional light shadow to shadow buffer
        gbuffer.unbind_as_source();
        lbuffer.unbind_as_target();
        vertex_array_.unbind();
    }

    if(auto dir_light = SCENE.get_directional_light().lock())
    {
        // Render shadow map
        math::mat4 lightMatrix;
        if(shadow_enabled_)
            lightMatrix = math::mat4(biasMatrix*smr_.render_directional_shadow_map()*V_inv);

    // ---- DIRECTIONAL LIGHT PASS
        gbuffer.bind_as_source();
        lbuffer.bind_as_target();

        vertex_array_.bind();
        if(lighting_enabled_)
            GFX::enable_blending();
        else
            GFX::disable_blending();
        GFX::cull_back();

        math::mat4 Id;
        Id.init_identity();
        lpass_dir_shader_.use();
        lpass_dir_shader_.send_uniform(H_("rd.b_lighting_enabled"), lighting_enabled_);
        // view position uniform
        //lpass_dir_shader_.send_uniform(H_("rd.v3_viewPos"), SCENE.get_camera()->get_position());
        // G-Buffer texture samplers
        lpass_dir_shader_.send_uniforms(*pgbuffer);
#ifdef __EXPERIMENTAL_POS_RECONSTRUCTION__
        // For position reconstruction
        lpass_dir_shader_.send_uniform(H_("rd.v4_proj_params"), proj_params);
#endif
        // Bright pass threshold
        lpass_dir_shader_.send_uniform(H_("rd.f_bright_threshold"), 1.0f);
        // Screen size
        lpass_dir_shader_.send_uniform(H_("rd.v2_screenSize"),
                                           math::vec2(gbuffer.get_width(),gbuffer.get_height()));
        // Light uniforms
        lpass_dir_shader_.send_uniform(H_("lt.v3_lightPosition"), V.submatrix(3,3)*dir_light->get_position());
        lpass_dir_shader_.send_uniforms(dir_light);
        lpass_dir_shader_.send_uniform(H_("m4_ModelViewProjection"), Id);

        // Shadow map
        lpass_dir_shader_.send_uniform(H_("rd.b_shadow_enabled"), shadow_enabled_);
        if(shadow_enabled_)
        {
            pshadow->bind(SHADOW_TEX,0); // Bind shadow[0] to texture unit SHADOW_TEX
            lpass_dir_shader_.send_uniform<int>(H_("shadowTex"), SHADOW_TEX);
            lpass_dir_shader_.send_uniform(H_("m4_LightSpace"), lightMatrix);
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
            pshadow->generate_mipmaps(0);
#else
            lpass_dir_shader_.send_uniform(H_("rd.v2_shadowTexelSize"), ShadowMapRenderer::SHADOW_TEXEL_SIZE);
            lpass_dir_shader_.send_uniform(H_("rd.f_shadowBias"), SCENE.get_shadow_bias() * ShadowMapRenderer::SHADOW_TEXEL_SIZE.x());
#endif
        }

        // SSAO
        if(SSAO_enabled_)
        {
            auto pssao = Texture::get_named_texture(H_("SSAObuffer")).lock();
            pssao->bind(SSAO_TEX,0); // Bind ssao[0] to texture unit SSAO_TEX
            lpass_dir_shader_.send_uniform<int>(H_("SSAOTex"), SSAO_TEX);
        }
        lpass_dir_shader_.send_uniform(H_("rd.b_enableSSAO"), SSAO_enabled_);

        buffer_unit_.draw(QUAD_NE(), QUAD_OFFSET());
        lpass_dir_shader_.unuse();

        GFX::disable_blending();
        GFX::disable_face_culling();

        //glDrawBuffer(GL_FRONT_AND_BACK);
        vertex_array_.unbind();
    }
}
