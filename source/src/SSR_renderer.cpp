#include "SSR_renderer.h"
#include "SSR_buffer.h"
#include "g_buffer.h"
#include "l_buffer.h"
#include "gfx_driver.h"
#include "scene.h"
#include "camera.h"
#include "texture.h"
#include "globals.h"
#include "logger.h"

namespace wcore
{

using namespace math;

SSRRenderer::SSRRenderer():
Renderer<Vertex3P>(),
SSR_shader_(ShaderResource("SSR.vert;SSR.frag")),
enabled_(true)
{
    load_geometry();
    SSRBuffer::Init(GLB.WIN_W, GLB.WIN_H);
}

SSRRenderer::~SSRRenderer()
{
    SSRBuffer::Kill();
}

void SSRRenderer::render(Scene* pscene)
{
    if(!enabled_) return;
    // For position reconstruction
    const mat4& P = pscene->get_camera().get_projection_matrix(); // Camera Projection matrix
    vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));

    GBuffer& gbuffer = GBuffer::Instance();
    LBuffer& lbuffer = LBuffer::Instance();
    SSRBuffer& ssrbuffer = SSRBuffer::Instance();


    SSR_shader_.use();
    SSR_shader_.send_uniform("rd.v2_texelSize"_h, vec2(1.0f/ssrbuffer.get_width(),1.0f/ssrbuffer.get_height()));
    SSR_shader_.send_uniform("rd.v4_proj_params"_h, proj_params);

    ssrbuffer.bind_as_target();
    GFX::clear_color();

    vertex_array_.bind();
    buffer_unit_.draw(2, 0);

    ssrbuffer.unbind_as_target();
    SSR_shader_.unuse();
}

} // namespace wcore
