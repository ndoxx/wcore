#include <iostream>

#include "gfx_driver.h"
#include "config.h"
#include "geometry_renderer.h"
#include "scene.h"
#include "camera.h"
#include "vertex_format.h"
#include "model.h"
#include "math3d.h"
#include "g_buffer.h"
#include "bounding_boxes.h"
#include "material.h"

namespace wcore
{

using namespace math;

GeometryRenderer::GeometryRenderer():
Renderer<Vertex3P3N3T2U>(),
geometry_pass_shader_(ShaderResource("gpass.vert;gpass.geom;gpass.frag")),
wireframe_mix_(0.0f),
allow_normal_mapping_(true),
allow_parallax_mapping_(true)
{
    CONFIG.get(H_("root.render.override.allow_normal_mapping"), allow_normal_mapping_);
    CONFIG.get(H_("root.render.override.allow_parallax_mapping"), allow_parallax_mapping_);
}

void GeometryRenderer::render()
{
    // Get camera matrices
    mat4 V = SCENE.get_camera()->get_view_matrix();       // Camera View matrix
    mat4 P = SCENE.get_camera()->get_projection_matrix(); // Camera Projection matrix
    mat4 PV = P*V;

    GFX::disable_blending();
    GFX::enable_depth_testing();
    GFX::unlock_depth_buffer();
    GFX::cull_back();
    geometry_pass_shader_.use();
    // Wireframe mix
    geometry_pass_shader_.send_uniform(H_("rd.f_wireframe_mix"), wireframe_mix_);
    // Camera (eye) position
    //geometry_pass_shader_.send_uniform(H_("rd.v3_viewPos"), SCENE.get_camera()->get_position());
    // Draw to G-Buffer
    GBuffer::Instance().bind_as_target();

    GFX::clear_color_depth();
    // Bind VAO, draw, unbind VAO
    SCENE.draw_models([&](std::shared_ptr<Model> pmodel)
    {
        // Get model matrix and compute products
        mat4 M = pmodel->get_model_matrix();
        mat4 MV = V*M;
        mat4 MVP = PV*M;

        // normal matrix for light calculation
        //geometry_pass_shader_.send_uniform(H_("tr.m3_Normal"), M.submatrix(3,3)); // Transposed inverse of M if non uniform scales
        geometry_pass_shader_.send_uniform(H_("tr.m3_Normal"), MV.submatrix(3,3)); // Transposed inverse of M if non uniform scales
        // model matrix
        //geometry_pass_shader_.send_uniform(H_("tr.m4_Model"), M);
        geometry_pass_shader_.send_uniform(H_("tr.m4_ModelView"), MV);
        // MVP matrix
        geometry_pass_shader_.send_uniform(H_("tr.m4_ModelViewProjection"), MVP);
        // material uniforms
        geometry_pass_shader_.send_uniforms(pmodel->get_material());
        // overrides
        if(!allow_normal_mapping_)
            geometry_pass_shader_.send_uniform(H_("mt.b_use_normal_map"), false);
        if(!allow_parallax_mapping_)
            geometry_pass_shader_.send_uniform(H_("mt.b_use_parallax_map"), false);
        if(pmodel->get_material().is_textured())
        {
            // bind current material texture units if any
            pmodel->get_material().bind_texture();
        }
    },
    [&](std::shared_ptr<Model> pmodel) // evaluator predicate
    {
        // Non cullable models are passed
        if(!pmodel->can_frustum_cull())
            return true;
/*
        // Get model AABB
        AABB& aabb = pmodel->get_AABB();
        // Frustum culling
        return SCENE.get_camera()->frustum_collides(aabb);
*/
        // Get model OBB
        OBB& obb = pmodel->get_OBB();
        // Frustum culling
        return SCENE.get_camera()->frustum_collides(obb);

    },
    wcore::ORDER::FRONT_TO_BACK);

    GBuffer::Instance().unbind_as_target();
    geometry_pass_shader_.unuse();

    // Lock depth buffer (read only)
    GFX::lock_depth_buffer();
    GFX::disable_depth_testing();
}

}
