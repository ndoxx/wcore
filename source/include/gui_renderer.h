#ifndef GUI_RENDERER_H
#define GUI_RENDERER_H

#include "renderer.h"
#include "shader.h"
#include "math3d.h"

namespace wcore
{

class Material;
class MaterialFactory;
struct Vertex2P2U;

struct CursorProperties
{
    CursorProperties(bool active, Material* material):
    active(active),
    material(material),
    color(0.5f,0.9f,1.0f),
    position(500),
    scale(1.0f)
    {

    }
    ~CursorProperties();

    bool active;
    Material* material;
    math::vec3 color;
    math::vec2 position;
    float scale;
};

class GuiRenderer : public Renderer
{
public:
    GuiRenderer();
    virtual ~GuiRenderer();

    virtual void render(Scene* pscene) override;
    inline void render_cursor(bool value)  { cursor_props_.active = value; }
    inline void toggle_cursor()  { cursor_props_.active = !cursor_props_.active; }
    inline bool is_cursor_rendered() const { return cursor_props_.active; }
    inline void set_cursor_position(float x, float y) { cursor_props_.position = math::vec2(x,y); }

    void set_cursor_hue(float hue);

private:
    Shader cursor_shader_;
    MaterialFactory* material_factory_;
    CursorProperties cursor_props_;

    void load_geometry();
};

} // namespace wcore

#endif // GUI_RENDERER_H
