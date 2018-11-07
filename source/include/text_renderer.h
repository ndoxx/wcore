#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>
#include <map>
#include <string>
#include <queue>

#include "utils.h"
#include "renderer.hpp"
#include "math3d.h"
#include "shader.h"

struct Character
{
    GLuint tex_ID;  // ID handle of the glyph texture
    long advance;   // Offset to advance to next glyph
    unsigned int size_w; // Size of glyph
    unsigned int size_h;
    int bearing_x; // Offset from baseline to left/top of glyph
    int bearing_y;
};

struct LineInfo
{
    std::string text;
    hash_t face;
    float x;
    float y;
    float scale;
    math::vec3 color;
};

struct Vertex2P2U;
class TextRenderer : public Renderer<Vertex2P2U>
{
private:
    FT_Library ft_;
    std::unordered_map<hash_t, FT_Face> faces_;
    std::unordered_map<hash_t, std::map<char, Character>> charsets_;
    std::queue<LineInfo> line_queue_;
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

    inline void schedule_for_drawing(const std::string& text,
                                     hash_t face,
                                     float x,
                                     float y,
                                     float scale=1.0f,
                                     math::vec3 color=math::vec3(1,1,1));
    virtual void render() override;

    inline void set_face(hash_t face_name);
};

inline void TextRenderer::schedule_for_drawing(const std::string& text,
                                               hash_t face,
                                               float x,
                                               float y,
                                               float scale,
                                               math::vec3 color)
{
    line_queue_.push({text, face, x, y, scale, color});
}

inline void TextRenderer::set_face(hash_t face_name)
{
    current_face_ = face_name;
}


#endif // TEXT_RENDERER_H
