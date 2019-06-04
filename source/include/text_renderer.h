#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <unordered_map>
#include <map>
#include <string>
#include <queue>
#include <memory>

#include "wtypes.h"
#include "renderer.h"
#include "math3d.h"
#include "shader.h"

namespace wcore
{

class TextRenderer : public Renderer
{
private:
    struct FontLibImpl;
    std::shared_ptr<FontLibImpl> pimpl_;

    Shader text_shader_;
    hash_t current_face_;

public:
    TextRenderer();
    virtual ~TextRenderer();

    void load_geometry();

    void load_face(const char* fontname,
                   uint32_t height = 32,
                   uint32_t width = 0);

    void render_line(const std::string& text, float x, float y, float scale, math::vec3 color);

    void schedule_for_drawing(const std::string& text,
                              hash_t face,
                              float x,
                              float y,
                              float scale=1.0f,
                              math::vec3 color=math::vec3(1,1,1));

    virtual void render(Scene* pscene) override;

    inline void set_face(hash_t face_name);
};

inline void TextRenderer::set_face(hash_t face_name)
{
    current_face_ = face_name;
}

}

#endif // TEXT_RENDERER_H
