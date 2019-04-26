#include <memory>

#include "forward_renderer.h"
#include "gfx_driver.h"
#include "g_buffer.h"
#include "l_buffer.h"
#include "scene.h"
#include "math3d.h"
#include "model.h"
#include "material.h"
#include "camera.h"
#include "sky.h"
#include "w_symbols.h"

namespace wcore
{

using namespace math;

ForwardRenderer::ForwardRenderer():
forward_stage_shader_(ShaderResource("forwardstage.vert;forwardstage.frag")),
skybox_shader_(ShaderResource("skybox.vert;skybox.frag")),
active_(true)
{

}

void ForwardRenderer::load_geometry()
{

}


void ForwardRenderer::render(Scene* pscene)
{
    if(!active_)
        return;

    // Get camera matrices
    mat4 V = pscene->get_camera().get_view_matrix();       // Camera View matrix
    mat4 P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    mat4 PV = P*V;

    GFX::enable_depth_testing();

    forward_stage_shader_.use();
    // Bind VAO, draw, unbind VAO
    LBuffer::Instance().bind_as_target();
    // Draw transparent geometry
    GFX::enable_blending();
    GFX::set_std_blending();

    pscene->draw_models([&](const Model& model)
    {
        // Get model matrix and compute products
        mat4 M = const_cast<Model&>(model).get_model_matrix();
        mat4 MVP = PV*M;

        // Transform
        forward_stage_shader_.send_uniform("tr.m4_ModelViewProjection"_h, MVP);
        // Material
        const Material& material = model.get_material();
        if(material.is_textured())
        {

        }
        else
        {
            forward_stage_shader_.send_uniform("mt.v4_tint"_h, vec4(material.get_albedo(),
                                                                  material.get_alpha()));
        }
    },
    wcore::DEFAULT_MODEL_EVALUATOR,
    wcore::ORDER::BACK_TO_FRONT,
    wcore::MODEL_CATEGORY::TRANSPARENT);

    GFX::disable_blending();
    forward_stage_shader_.unuse();

    // Draw skybox
    if(pscene->has_skybox())
    {
        glDepthFunc(GL_LEQUAL);
        skybox_shader_.use();
        const SkyBox& skybox = pscene->get_skybox();
        // Bind skybox's cubemap
        skybox.bind();

        // View matrix without the translational part
        mat4 view(V);
        view[12] = 0.f;
        view[13] = 0.f;
        view[14] = 0.f;
        mat4 view_proj = P*view;

        skybox_shader_.send_uniform<int>("skyboxTex"_h, 0);
        skybox_shader_.send_uniform("m4_view_projection"_h, view_proj);
        skybox.draw();

        skybox_shader_.unuse();
        glDepthFunc(GL_LESS);
    }
    LBuffer::Instance().unbind_as_target();

    // Restore state
    GFX::disable_depth_testing();
}

}
