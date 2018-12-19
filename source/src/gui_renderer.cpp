#include "gui_renderer.h"
#include "gfx_driver.h"
#include "vertex_format.h"
#include "material.h"
#include "material_factory.h"
#include "config.h"
#include "globals.h"
#include "colors.h"

#include "logger.h"

namespace wcore
{

using namespace math;

CursorProperties::~CursorProperties()
{
    delete material;
}

GuiRenderer::GuiRenderer():
Renderer<Vertex2P2U>(),
cursor_shader_(ShaderResource("cursor.vert;cursor.frag")),
material_factory_(new MaterialFactory("gui_assets.xml")),
cursor_props_(false, material_factory_->make_material(H_("cursor")))
{
    load_geometry();
}

GuiRenderer::~GuiRenderer()
{
    delete material_factory_;
}

void GuiRenderer::load_geometry()
{
    float xpos = 0.0f;
    float ypos = 0.0f;
    float w = 1.0f;
    float h = 1.0f;
    Mesh<Vertex2P2U> quadmesh;
    quadmesh._emplace_vertex(vec2(xpos,   ypos  ), vec2(0, 0));
    quadmesh._emplace_vertex(vec2(xpos+w, ypos  ), vec2(1, 0));
    quadmesh._emplace_vertex(vec2(xpos+w, ypos+h), vec2(1, 1));
    quadmesh._emplace_vertex(vec2(xpos,   ypos+h), vec2(0, 1));
    quadmesh._push_triangle(0,  1,  2);
    quadmesh._push_triangle(0,  2,  3);

    buffer_unit_.submit(quadmesh);
    buffer_unit_.upload();
}

void GuiRenderer::set_cursor_hue(float hue)
{
    cursor_props_.color = color::hsl2rgb(vec3(hue, 1.0f, 0.5f));
}


void GuiRenderer::render()
{
    GFX::bind_default_frame_buffer();
    GFX::viewport(0,0,GLB.WIN_W,GLB.WIN_H);
    GFX::enable_blending();
    GFX::set_std_blending();
    vertex_array_.bind();

    // Render cursor if needed
    if(cursor_props_.active && CONFIG.is(H_("root.gui.cursor.custom")))
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
        cursor_shader_.send_uniform(H_("v3_color"), cursor_props_.color);
        cursor_shader_.send_uniform(H_("m4_transform"), transform);
        cursor_shader_.send_uniform(H_("m4_transform"), transform);
        cursor_shader_.send_uniform<int>(H_("inputTex"), 0);
        buffer_unit_.draw(2, 0);

        cursor_shader_.unuse();
    }
}

} // namespace wcore
