#include <cstdlib>

#include "debug_renderer.h"
#include "gfx_driver.h"
#include "mesh.hpp"
#include "vertex_format.h"
#include "bounding_boxes.h"
#include "model.h"
#include "material.h"
#include "lights.h"
#include "math3d.h"
#include "scene.h"
#include "g_buffer.h"
#include "camera.h"
#include "globals.h"
#include "logger.h"
#ifndef __DISABLE_EDITOR__
#include "editor.h"
#endif

namespace wcore
{

using namespace math;

DebugRenderer::DebugRenderer():
Renderer<Vertex3P>(GL_LINES),
line_shader_(ShaderResource("line.vert;line.frag")),
display_line_models_(true),
light_proxy_scale_(1.0f),
light_display_mode_(0),
bb_display_mode_(0),
enable_depth_test_(true),
show_static_octree_(false)
{
    load_geometry();
}

static size_t CUBE_OFFSET = 0;
static size_t SPHERE_OFFSET = 0;
static size_t SEG_X_OFFSET = 0;
static size_t SEG_Y_OFFSET = 0;
static size_t SEG_Z_OFFSET = 0;
static size_t CROSS3_OFFSET = 0;
static size_t CUBE_NE = 0;
static size_t SPHERE_NE = 0;
static size_t SEG_X_NE = 0;
static size_t SEG_Y_NE = 0;
static size_t SEG_Z_NE = 0;
static size_t CROSS3_NE = 0;

void DebugRenderer::load_geometry()
{
    Mesh<Vertex3P>* cube_mesh   = factory::make_cube_3P();
    Mesh<Vertex3P>* sphere_mesh = factory::make_uv_sphere_3P(4, 7, true);
    Mesh<Vertex3P>* seg_mesh_x  = factory::make_segment_x_3P();
    Mesh<Vertex3P>* seg_mesh_y  = factory::make_segment_y_3P();
    Mesh<Vertex3P>* seg_mesh_z  = factory::make_segment_z_3P();
    Mesh<Vertex3P>* cross3_mesh = factory::make_cross3D_3P();
    buffer_unit_.submit(*cube_mesh);
    buffer_unit_.submit(*sphere_mesh);
    buffer_unit_.submit(*seg_mesh_x);
    buffer_unit_.submit(*seg_mesh_y);
    buffer_unit_.submit(*seg_mesh_z);
    buffer_unit_.submit(*cross3_mesh);
    buffer_unit_.upload();
    CUBE_OFFSET   = cube_mesh->get_buffer_offset();
    SPHERE_OFFSET = sphere_mesh->get_buffer_offset();
    SEG_X_OFFSET = seg_mesh_x->get_buffer_offset();
    SEG_Y_OFFSET = seg_mesh_y->get_buffer_offset();
    SEG_Z_OFFSET = seg_mesh_z->get_buffer_offset();
    CROSS3_OFFSET = cross3_mesh->get_buffer_offset();
    CUBE_NE   = cube_mesh->get_n_elements();
    SPHERE_NE = sphere_mesh->get_n_elements();
    SEG_X_NE = seg_mesh_x->get_n_elements();
    SEG_Y_NE = seg_mesh_y->get_n_elements();
    SEG_Z_NE = seg_mesh_z->get_n_elements();
    CROSS3_NE = cross3_mesh->get_n_elements();
    delete cube_mesh;
    delete sphere_mesh;
    delete seg_mesh_x;
    delete seg_mesh_y;
    delete seg_mesh_z;
    delete cross3_mesh;
}

void DebugRenderer::render(Scene* pscene)
{
#ifdef __DEBUG__
    // Get camera matrices
    mat4 V = pscene->get_camera().get_view_matrix();       // Camera View matrix
    mat4 P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    mat4 PV = P*V;

    GBuffer::Instance().blit_depth_to_screen(GLB.WIN_W, GLB.WIN_H);
    if(enable_depth_test_)
        GFX::enable_depth_testing();

    line_shader_.use();

    // pscene->OBJECTS AABB/OBB
    pscene->traverse_models([&](const Model& model, uint32_t chunk_index)
    {
        // Get model matrix and compute products
        if(bb_display_mode_ == 1 || model.debug_display_opts_.is_enabled(DebugDisplayOptions::AABB))
        {
            AABB& aabb = const_cast<Model&>(model).get_AABB();
            if(!pscene->get_camera().frustum_collides(aabb)) return;
            mat4 M = aabb.get_model_matrix();
            line_shader_.send_uniform("v4_line_color"_h, vec4(0,1,0,0));
            mat4 MVP = PV*M;
            line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
            buffer_unit_.draw(CUBE_NE, CUBE_OFFSET);
        }
        if(bb_display_mode_ == 2 || model.debug_display_opts_.is_enabled(DebugDisplayOptions::OBB))
        {
            OBB& obb = const_cast<Model&>(model).get_OBB();
            if(!pscene->get_camera().frustum_collides(obb)) return;
            mat4 M = obb.get_model_matrix();
            line_shader_.send_uniform("v4_line_color"_h, vec4(0,0,1,0));
            mat4 MVP = PV*M;
            line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
            buffer_unit_.draw(CUBE_NE, CUBE_OFFSET);
        }
        if(bb_display_mode_ == 3 || model.debug_display_opts_.is_enabled(DebugDisplayOptions::ORIGIN))
        {
            OBB& obb = const_cast<Model&>(model).get_OBB();
            if(!pscene->get_camera().frustum_collides(obb)) return;
            mat4 M = const_cast<Model&>(model).get_transformation().get_rotation_translation_matrix();
            mat4 MVP = PV*M;
            line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
            line_shader_.send_uniform("v4_line_color"_h, vec4(1,0,0,0));
            buffer_unit_.draw(SEG_X_NE, SEG_X_OFFSET);
            line_shader_.send_uniform("v4_line_color"_h, vec4(0,1,0,0));
            buffer_unit_.draw(SEG_Y_NE, SEG_Y_OFFSET);
            line_shader_.send_uniform("v4_line_color"_h, vec4(0,0,1,0));
            buffer_unit_.draw(SEG_Z_NE, SEG_Z_OFFSET);
        }
    },
    wcore::DEFAULT_MODEL_EVALUATOR,
    wcore::ORDER::IRRELEVANT,
    wcore::MODEL_CATEGORY::OPAQUE);

#ifndef __DISABLE_EDITOR__
    // EDITOR SELECTION
    if(auto* psel = pscene->locate_editor()->get_model_selection())
    {
        OBB& obb = const_cast<Model*>(psel)->get_OBB();
        mat4 M = obb.get_model_matrix();
        line_shader_.send_uniform("v4_line_color"_h, vec4(1,0.8,0,0));
        mat4 MVP = PV*M;
        line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
        buffer_unit_.draw(CUBE_NE, CUBE_OFFSET);
    }
#endif

    // LIGHT PROXY GEOMETRY
    if(light_display_mode_ > 0)
    {
        pscene->traverse_lights([&](const Light& light, uint32_t chunk_index)
        {
            // Get model matrix and compute products
            // Scale model in display mode 2 only
            mat4 M(light.get_model_matrix(light_display_mode_==2));
            mat4 MVP;
            if(light_proxy_scale_!=1.f)
            {
                mat4 S;
                S.init_scale(light_proxy_scale_);
                MVP = PV*M*S;
            }
            else
                MVP = PV*M;

            line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
            line_shader_.send_uniform("v4_line_color"_h, vec4(light.get_color().normalized()));
            buffer_unit_.draw(SPHERE_NE, SPHERE_OFFSET);
        },
        [&](const Light& light)
        {
            return light.is_in_frustum(pscene->get_camera());
        });
    }

    // OCTREE DISPLAY
    if(show_static_octree_)
    {
        float far = pscene->get_camera().get_far();
        GFX::enable_blending();
        GFX::set_std_blending();
        auto&& static_octree = pscene->get_static_octree();
        // For each bounding region that is visible
        static_octree.traverse_bounds_range(pscene->get_camera().get_frustum_box(),
        [&](auto&& bound)
        {
            // Display bounding cube
            mat4 M;
            M.init_scale(2.f*bound.half);
            translate_matrix(M, bound.mid_point);
            mat4 MVP = PV*M;
            float alpha = 1.0f-std::min((bound.mid_point-pscene->get_camera().get_position()).norm()/far,1.0f);
            line_shader_.send_uniform("v4_line_color"_h, vec4(1,0,0,alpha));
            line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
            buffer_unit_.draw(CUBE_NE, CUBE_OFFSET);
        });
        GFX::disable_blending();
    }

    // DRAW REQUESTS
    auto it = draw_requests_.begin();
    while(it != draw_requests_.end())
    {
        // Remove dead requests
        bool alive = (--(*it).ttl >= 0);
        if (!alive)
        {
            draw_requests_.erase(it++);  // alternatively, it = draw_requests_.erase(it);
        }
        // Display the required primitive
        else
        {
            // Get model matrix and compute products
            mat4 MVP(PV*(*it).model_matrix);

            line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
            line_shader_.send_uniform("v4_line_color"_h, vec4((*it).color));

            if((*it).type == DebugDrawRequest::SEGMENT)
            {
                buffer_unit_.draw(SEG_X_NE, SEG_X_OFFSET);
            }
            else if((*it).type == DebugDrawRequest::CUBE)
            {
                buffer_unit_.draw(CUBE_NE, CUBE_OFFSET);
            }
            else if((*it).type == DebugDrawRequest::SPHERE)
            {
                buffer_unit_.draw(SPHERE_NE, SPHERE_OFFSET);
            }
            else if((*it).type == DebugDrawRequest::CROSS3)
            {
                glLineWidth(2.5f);
                buffer_unit_.draw(CROSS3_NE, CROSS3_OFFSET);
                glLineWidth(1.0f);
            }
            ++it;
        }
    }


    // LINE MODELS
    if(display_line_models_)
    {
        pscene->draw_line_models([&](pLineModel pmodel)
        {
            mat4 M(pmodel->get_model_matrix());
            mat4 MVP = PV*M;

            line_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
            line_shader_.send_uniform("v4_line_color"_h, vec4(pmodel->get_material().get_albedo()));
        });
    }
    line_shader_.unuse();

    if(enable_depth_test_)
        GFX::disable_depth_testing();
#endif
}

void DebugRenderer::show_selection_neighbors(Scene* pscene, float radius)
{
#ifndef __DISABLE_EDITOR__
    if(auto* psel = pscene->locate_editor()->get_model_selection())
    {
        Sphere bounds(psel->get_position(), radius);
        auto&& static_octree = pscene->get_static_octree();
        static_octree.traverse_range(bounds,
        [&](auto&& obj)
        {
            // Display bounding cube
            mat4 M(obj.data.model->get_OBB().get_model_matrix());
            request_draw_cube(M, 60*5, vec3(1,0,1));
        });
    }
#endif
}

void DebugRenderer::request_draw_segment(const math::vec3& world_start,
                                         const math::vec3& world_end,
                                         int ttl,
                                         const math::vec3& color)
{
    DebugDrawRequest request;
    request.type = DebugDrawRequest::SEGMENT;
    request.ttl = ttl;
    request.color = color;
    request.color[3] = 1.0f;
    request.model_matrix = math::segment_transform(world_start, world_end);
    draw_requests_.push_back(request);
}

void DebugRenderer::request_draw_sphere(const math::vec3& world_pos,
                                        float radius,
                                        int ttl,
                                        const math::vec3& color)
{
    DebugDrawRequest request;
    request.type = DebugDrawRequest::SPHERE;
    request.ttl = ttl;
    request.color = color;
    request.color[3] = 1.0f;
    request.model_matrix = math::scale_translate(world_pos, radius);
    draw_requests_.push_back(request);
}

void DebugRenderer::request_draw_cross3(const math::vec3& world_pos,
                                        float radius,
                                        int ttl,
                                        const math::vec3& color)
{
    DebugDrawRequest request;
    request.type = DebugDrawRequest::CROSS3;
    request.ttl = ttl;
    request.color = color;
    request.color[3] = 1.0f;
    request.model_matrix = math::scale_translate(world_pos, radius);
    draw_requests_.push_back(request);
}

void DebugRenderer::request_draw_cube(const math::mat4& model_patrix,
                                      int ttl,
                                      const math::vec3& color)
{
    DebugDrawRequest request;
    request.type = DebugDrawRequest::CUBE;
    request.ttl = ttl;
    request.color = color;
    request.color[3] = 1.0f;
    request.model_matrix = model_patrix;
    draw_requests_.push_back(request);
}

}
