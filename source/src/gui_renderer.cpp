#include "gui_renderer.h"
#include "gfx_api.h"
#include "vertex_format.h"
#include "material.h"
#include "material_factory.h"
#include "config.h"
#include "globals.h"
#include "colors.h"
#include "geometry_common.h"

#include "logger.h"

namespace wcore
{

using namespace math;

CursorProperties::~CursorProperties()
{
    delete material;
}

GuiRenderer::GuiRenderer():
cursor_shader_(ShaderResource("cursor.vert;cursor.frag")),
material_factory_(new MaterialFactory("gui_assets.xml")),
cursor_props_(false, material_factory_->make_material("cursor"_h))
{

}

GuiRenderer::~GuiRenderer()
{
    delete material_factory_;
}

void GuiRenderer::set_cursor_hue(float hue)
{
    cursor_props_.color = color::hsl2rgb(vec3(hue, 1.0f, 0.5f));
}


void GuiRenderer::render(Scene* pscene)
{
    Gfx::bind_default_frame_buffer();
    Gfx::viewport(0,0,GLB.WIN_W,GLB.WIN_H);
    Gfx::set_std_blending();

    // Render cursor if needed
    if(cursor_props_.active && CONFIG.is("root.gui.cursor.custom"_h))
    {
        // Screen-space scale and translate
        float cursor_size = 64.0f * cursor_props_.scale / GLB.WIN_H;
        float aspect = GLB.WIN_W/(1.0f*GLB.WIN_H);
        float w = cursor_size;
        float h = cursor_size/aspect;
        float xpos = (2.0f*cursor_props_.position.x()) - 1.0f;
        float ypos = (2.0f*cursor_props_.position.y() - cursor_size/aspect) - 1.0f;
        mat4 transform(w, 0, 0, xpos,
                       0, h, 0, ypos,
                       0, 0, 1, 0,
                       0, 0, 0, 1);
        cursor_props_.material->bind_texture();

        cursor_shader_.use();
        cursor_shader_.send_uniform("v3_color"_h, cursor_props_.color);
        cursor_shader_.send_uniform("m4_transform"_h, transform);
        cursor_shader_.send_uniform("m4_transform"_h, transform);
        cursor_shader_.send_uniform<int>("inputTex"_h, 0);
        CGEOM.draw("screen_quad"_h);

        cursor_shader_.unuse();
    }

    Gfx::disable_blending();
}

} // namespace wcore
