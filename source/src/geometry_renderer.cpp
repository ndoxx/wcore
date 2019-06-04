#include <iostream>

#include "gfx_api.h"
#include "config.h"
#include "geometry_renderer.h"
#include "scene.h"
#include "camera.h"
#include "vertex_format.h"
#include "model.h"
#include "terrain_patch.h"
#include "math3d.h"
#include "bounding_boxes.h"
#include "material.h"
#include "texture.h"
#include "buffer_module.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;


GeometryRenderer::GeometryRenderer():
geometry_pass_shader_(ShaderResource("gpass.vert;gpass.geom;gpass.frag")),
terrain_shader_(ShaderResource("gpass.vert;gpass.geom;gpass.frag", "VARIANT_SPLAT")),
null_shader_(ShaderResource("null.vert;null.frag")),
wireframe_mix_(0.0f),
min_parallax_distance_(20.f),
allow_normal_mapping_(true),
allow_parallax_mapping_(true)
{
    CONFIG.get("root.render.override.allow_normal_mapping"_h, allow_normal_mapping_);
    CONFIG.get("root.render.override.allow_parallax_mapping"_h, allow_parallax_mapping_);

    Gfx::device->set_clear_color(0.f,0.f,0.f,1.f);
}

void GeometryRenderer::render(Scene* pscene)
{
    auto& g_buffer = GMODULES::GET("gbuffer"_h);

    // Get camera matrices
    mat4 V = pscene->get_camera().get_view_matrix();       // Camera View matrix
    mat4 P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    mat4 PV = P*V;
    vec3 campos = pscene->get_camera().get_position();

    Gfx::device->disable_blending();
    Gfx::device->set_depth_test_enabled(true);
    Gfx::device->set_depth_lock(false);

    Shader* shader = &geometry_pass_shader_;

    Gfx::device->set_cull_mode(CullMode::Back);
    shader->use();
    // Wireframe mix
    shader->send_uniform("rd.f_wireframe_mix"_h, wireframe_mix_);
    // Camera (eye) position
    //shader->send_uniform("rd.v3_viewPos"_h, pscene->get_camera()->get_position());
    // Draw to G-Buffer
    g_buffer.bind_as_target();

    Gfx::device->clear(CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG); // Suppressed valgrind false positive in valgrind.supp

    pscene->draw_models([&](const Model& model)
    {
        // Get model matrix and compute products
        mat4 M = const_cast<Model&>(model).get_model_matrix();
        mat4 MV = V*M;
        mat4 MVP = PV*M;

        // normal matrix for light calculation
        //shader->send_uniform("tr.m3_Normal"_h, M.submatrix(3,3)); // Transposed inverse of M if non uniform scales
        shader->send_uniform("tr.m3_Normal"_h, MV.submatrix(3,3)); // Transposed inverse of M if non uniform scales
        // model matrix
        //shader->send_uniform("tr.m4_Model"_h, M);
        shader->send_uniform("tr.m4_ModelView"_h, MV);
        // MVP matrix
        shader->send_uniform("tr.m4_ModelViewProjection"_h, MVP);
        // material uniforms
        shader->send_uniforms(model.get_material());
        // overrides
        if(!allow_normal_mapping_)
            shader->send_uniform("mt.b_use_normal_map"_h, false);
        if(!allow_parallax_mapping_)
            shader->send_uniform("mt.b_use_parallax_map"_h, false);
        else
        {
            // use parallax mapping only if object is close enough
            float dist = (model.get_position()-campos).norm();
            shader->send_uniform("mt.b_use_parallax_map"_h, (dist < min_parallax_distance_));
        }
        if(model.get_material().is_textured())
        {
            // bind current material texture units if any
            model.get_material().bind_texture();
        }
    },
    [&](const Model& model) // evaluator predicate
    {
        return model.is_visible(); // Visibility is evaluated during update by Scene::visibility_pass()
    },
    wcore::ORDER::FRONT_TO_BACK);
    shader->unuse();


    // TERRAINS
    // Terrains are heavily occluded by the static geometry on top,
    // so we draw them last so as to maximize depth test fails
    pscene->draw_terrains([&](const TerrainChunk& terrain)
    {
        if(terrain.is_multi_textured())
            shader = &terrain_shader_;
        else
            shader = &geometry_pass_shader_;

        shader->use();
        shader->send_uniform("rd.f_wireframe_mix"_h, wireframe_mix_);

        // Get model matrix and compute products
        mat4 M = const_cast<TerrainChunk&>(terrain).get_model_matrix();
        mat4 MV = V*M;
        mat4 MVP = PV*M;

        // normal matrix for light calculation
        shader->send_uniform("tr.m3_Normal"_h, MV.submatrix(3,3)); // Transposed inverse of M if non uniform scales
        // model matrix
        shader->send_uniform("tr.m4_ModelView"_h, MV);
        // MVP matrix
        shader->send_uniform("tr.m4_ModelViewProjection"_h, MVP);
        // material uniforms
        shader->send_uniforms(terrain.get_material());
        if(terrain.get_material().is_textured())
        {
            // bind current material texture units if any
            terrain.get_material().bind_texture();
        }

        if(terrain.is_multi_textured())
        {
            shader->send_uniforms(terrain.get_alternative_material());
            if(terrain.get_alternative_material().is_textured())
            {
                terrain.get_alternative_material().bind_texture();
            }

            // Send splatmap
            const Texture& splatmap = terrain.get_splatmap();
            /*splatmap.bind(12,0);
            shader->send_uniform<int>("mt.splatTex"_h, 12);*/
            splatmap.bind(6,0);
            shader->send_uniform<int>("mt.splatTex"_h, 6);

            shader->send_uniform("f_inv_chunk_size"_h, 1.f/terrain.get_chunk_size());
        }

        // overrides
        if(!allow_normal_mapping_)
            shader->send_uniform("mt.b_use_normal_map"_h, false);
        if(!allow_parallax_mapping_)
            shader->send_uniform("mt.b_use_parallax_map"_h, false);
        else
        {
            // use parallax mapping only if object is close enough
            float dist = (terrain.get_position()-campos).norm();
            shader->send_uniform("mt.b_use_parallax_map"_h, (dist < min_parallax_distance_));
        }
    });
    shader->unuse();

    g_buffer.unbind_as_target();


    // EXP back face depth buffer ---------------------------------------------
    auto& bfd_buffer  = GMODULES::GET("backfaceDepthBuffer"_h);
    null_shader_.use();
    Gfx::device->set_cull_mode(CullMode::Front);
    bfd_buffer.bind_as_target();
    Gfx::device->clear(CLEAR_DEPTH_FLAG);

    pscene->draw_models([&](const Model& model)
    {
        // Get model matrix and compute products
        mat4 M = const_cast<Model&>(model).get_model_matrix();
        mat4 MVP = PV*M;

        // MVP matrix
        null_shader_.send_uniform("m4_ModelViewProjection"_h, MVP);
    },
    [&](const Model& model) // evaluator predicate
    {
        return model.is_visible(); // Visibility is evaluated during update by Scene::visibility_pass()
    },
    wcore::ORDER::FRONT_TO_BACK);

    pscene->draw_terrains([&](const TerrainChunk& terrain)
    {
        // Get model matrix and compute products
        mat4 M = const_cast<TerrainChunk&>(terrain).get_model_matrix();
        mat4 MVP = PV*M;

        // MVP matrix
        null_shader_.send_uniform("m4_ModelViewProjection"_h, MVP);
    });

    bfd_buffer.unbind_as_target();
    null_shader_.unuse();
    Gfx::device->set_cull_mode(CullMode::Back);
    // EXP back face depth buffer ---------------------------------------------


    // Lock depth buffer (read only)
    Gfx::device->set_depth_lock(true);
    Gfx::device->set_depth_test_enabled(false);
}

}
