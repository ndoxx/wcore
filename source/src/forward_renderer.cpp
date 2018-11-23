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
#include "w_symbols.h"

namespace wcore
{

using namespace math;

ForwardRenderer::ForwardRenderer():
Renderer<Vertex3P3N3T2U>(),
forward_stage_shader_(ShaderResource("forwardstage.vert;forwardstage.frag"))
{

}

void ForwardRenderer::load_geometry()
{

}


void ForwardRenderer::render()
{
    GBuffer& gbuffer = GBuffer::Instance();
    LBuffer& lbuffer = LBuffer::Instance();

    // Get camera matrices
    mat4 V = SCENE.get_camera()->get_view_matrix();       // Camera View matrix
    mat4 P = SCENE.get_camera()->get_projection_matrix(); // Camera Projection matrix
    mat4 PV = P*V;

    GFX::enable_depth_testing();

    forward_stage_shader_.use();
    // Bind VAO, draw, unbind VAO
    lbuffer.draw_to([&]()
    {
        // Draw transparent geometry
        GFX::enable_blending();
        GFX::set_std_blending();

        SCENE.draw_models([&](std::shared_ptr<Model> pmodel)
        {
            // Get model matrix and compute products
            mat4 M = pmodel->get_model_matrix();
            mat4 MVP = PV*M;

            // Transform
            forward_stage_shader_.send_uniform(H_("tr.m4_ModelViewProjection"), MVP);
            // Material
            const Material& material = pmodel->get_material();
            if(material.is_textured())
            {

            }
            else
            {
                forward_stage_shader_.send_uniform(H_("mt.v4_tint"), vec4(material.get_albedo(),
                                                                      material.get_alpha()));
            }
        },
        wcore::DEFAULT_MODEL_EVALUATOR,
        wcore::ORDER::BACK_TO_FRONT,
        wcore::MODEL_CATEGORY::TRANSPARENT);

        GFX::disable_blending();

        // Draw skybox
        // ...
    });
    forward_stage_shader_.unuse();

    // Restore state
    GFX::disable_depth_testing();
}

}
