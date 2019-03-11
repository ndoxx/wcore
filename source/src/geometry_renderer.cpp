#include <iostream>

#include "gfx_driver.h"
#include "config.h"
#include "geometry_renderer.h"
#include "scene.h"
#include "camera.h"
#include "vertex_format.h"
#include "model.h"
#include "terrain_patch.h"
#include "math3d.h"
#include "g_buffer.h"
#include "bounding_boxes.h"
#include "material.h"
#include "texture.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;


GeometryRenderer::GeometryRenderer():
Renderer<Vertex3P3N3T2U>(),
geometry_pass_shader_(ShaderResource("gpass.vert;gpass.geom;gpass.frag")),
terrain_shader_(ShaderResource("gpass.vert;gpass.geom;gpass.frag", "VARIANT_SPLAT")),
wireframe_mix_(0.0f),
min_parallax_distance_(20.f),
allow_normal_mapping_(true),
allow_parallax_mapping_(true)
{
    CONFIG.get("root.render.override.allow_normal_mapping"_h, allow_normal_mapping_);
    CONFIG.get("root.render.override.allow_parallax_mapping"_h, allow_parallax_mapping_);
}

static uint64_t texture_index;

void GeometryRenderer::render(Scene* pscene)
{
    // Get camera matrices
    mat4 V = pscene->get_camera().get_view_matrix();       // Camera View matrix
    mat4 P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    mat4 PV = P*V;
    vec3 campos = pscene->get_camera().get_position();

    GFX::disable_blending();
    GFX::enable_depth_testing();
    GFX::unlock_depth_buffer();
    GFX::cull_back();
    geometry_pass_shader_.use();
    // Wireframe mix
    geometry_pass_shader_.send_uniform("rd.f_wireframe_mix"_h, wireframe_mix_);
    // Camera (eye) position
    //geometry_pass_shader_.send_uniform("rd.v3_viewPos"_h, pscene->get_camera()->get_position());
    // Draw to G-Buffer
    GBuffer::Instance().bind_as_target();

    GFX::clear_color_depth();
    // Bind VAO, draw, unbind VAO
    pscene->draw_models([&](const Model& model)
    {
        // Get model matrix and compute products
        mat4 M = const_cast<Model&>(model).get_model_matrix();
        mat4 MV = V*M;
        mat4 MVP = PV*M;

        // normal matrix for light calculation
        //geometry_pass_shader_.send_uniform("tr.m3_Normal"_h, M.submatrix(3,3)); // Transposed inverse of M if non uniform scales
        geometry_pass_shader_.send_uniform("tr.m3_Normal"_h, MV.submatrix(3,3)); // Transposed inverse of M if non uniform scales
        // model matrix
        //geometry_pass_shader_.send_uniform("tr.m4_Model"_h, M);
        geometry_pass_shader_.send_uniform("tr.m4_ModelView"_h, MV);
        // MVP matrix
        geometry_pass_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
        // material uniforms
        geometry_pass_shader_.send_uniforms(model.get_material());
        // overrides
        if(!allow_normal_mapping_)
            geometry_pass_shader_.send_uniform("mt.b_use_normal_map"_h, false);
        if(!allow_parallax_mapping_)
            geometry_pass_shader_.send_uniform("mt.b_use_parallax_map"_h, false);
        else
        {
            // use parallax mapping only if object is close enough
            float dist = (model.get_position()-campos).norm();
            geometry_pass_shader_.send_uniform("mt.b_use_parallax_map"_h, (dist < min_parallax_distance_));
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
    geometry_pass_shader_.unuse();


    // TERRAINS
    // Terrains are heavily occluded by the static geometry on top,
    // so we draw them last so as to maximize depth test fails
    Shader* shader = nullptr;
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

    GBuffer::Instance().unbind_as_target();

    // Lock depth buffer (read only)
    GFX::lock_depth_buffer();
    GFX::disable_depth_testing();
}

// TMP for debugging
#ifndef __DISABLE_EDITOR__
void GeometryRenderer::generate_widget()
{
    if(!ImGui::Begin("Texture peek"))
    {
        ImGui::End();
        return;
    }

    ImGui::GetWindowDrawList()->AddImage((void*)texture_index,
                                         ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
                                         ImVec2(0, 1), ImVec2(1, 0));

    ImGui::End();
}
#endif

}
