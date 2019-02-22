#include "text_renderer.h"
#include "gfx_driver.h"
#include "logger.h"
#include "vertex_format.h"
#include "mesh.hpp"
#include "globals.h"
#include "io_utils.h"
#include "error.h"

namespace wcore
{

using namespace math;

TextRenderer::TextRenderer():
Renderer<Vertex2P2U>(),
ft_(),
text_shader_(ShaderResource("text.vert;text.frag"))
{
    if (FT_Init_FreeType(&ft_))
    {
        DLOGF("[TextRenderer] Could not init FreeType Library.", "text", Severity::CRIT);
        fatal("Could not init FreeType Library.");
    }

    load_geometry();
}

TextRenderer::~TextRenderer()
{
    for(auto p : faces_)
        FT_Done_Face(p.second);
    FT_Done_FreeType(ft_);
}

void TextRenderer::load_geometry()
{
    float xpos = 0.0f;
    float ypos = 0.0f;
    float w = 1.0f;
    float h = 1.0f;
    Mesh<Vertex2P2U> quadmesh;
    quadmesh._emplace_vertex(vec2(xpos,   ypos  ), vec2(0, 1));
    quadmesh._emplace_vertex(vec2(xpos+w, ypos  ), vec2(1, 1));
    quadmesh._emplace_vertex(vec2(xpos+w, ypos+h), vec2(1, 0));
    quadmesh._emplace_vertex(vec2(xpos,   ypos+h), vec2(0, 0));
    quadmesh._push_triangle(0,  1,  2);
    quadmesh._push_triangle(0,  2,  3);

    buffer_unit_.submit(quadmesh);
    buffer_unit_.upload();
}


void TextRenderer::load_face(const char* fontname,
                             uint32_t height,
                             uint32_t width)
{
    std::string font_file(fontname);
    font_file += ".ttf";
    fs::path file_path(io::get_file("root.folders.font"_h, font_file));

    std::string full_path_str(file_path.string());

    // Generate new face
    // TODO: use FT_Open_Face to load from memory, so we can use streams
    FT_Face face;
    if (FT_New_Face(ft_, full_path_str.c_str(), 0, &face))
    {
        DLOGE("[TextRenderer] Failed to load font: <p>" + full_path_str + "</p>", "text", Severity::CRIT);
        return;
    }

    // Set face size
    FT_Set_Pixel_Sizes(face, width, height);
    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate character glyphs
    std::map<char, Character> characters;
    for (unsigned char cc = 0; cc < 128; ++cc)
    {
        // Load character glyph
        if (FT_Load_Char(face, cc, FT_LOAD_RENDER))
        {
            DLOGW(std::string("[TextRenderer] Failed to load Glyph: \'") + std::to_string(cc) + "\'", "text", Severity::WARN);
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character =
        {
            texture,
            face->glyph->advance.x,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            face->glyph->bitmap_left,
            face->glyph->bitmap_top,

        };
        characters.insert(std::pair<char, Character>(cc, character));
    }

    hash_t hname(H_(fontname));
    faces_.insert(std::make_pair(hname, face));
    charsets_.insert(std::make_pair(hname, characters));

    set_face(hname);

#ifdef __DEBUG__
    DLOGN("[TextRenderer] New face: <n>" + std::string(fontname) + "</n>", "text", Severity::LOW);
    DLOGI("from file: <p>" + full_path_str + "</p>", "text", Severity::LOW);
#endif

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Restore byte-alignment state
}

void TextRenderer::render_line(const std::string& text, float x, float y, float scale, math::vec3 color)
{
    x *= 2;
    y *= 2;
    GFX::bind_default_frame_buffer();
    GFX::viewport(0,0,GLB.WIN_W,GLB.WIN_H);
    text_shader_.use();
    text_shader_.send_uniform("v3_textColor"_h, color);

    GFX::enable_blending();
    GFX::set_std_blending();
    vertex_array_.bind();
    // Iterate through all characters
    std::string::const_iterator itc;
    for (itc = text.begin(); itc != text.end(); ++itc)
    {
        const Character& ch = charsets_[current_face_][char(*itc)];

        float xpos = (x + ch.bearing_x * scale)/GLB.WIN_W -1.0f;
        float ypos = (y - (ch.size_h - ch.bearing_y) * scale)/GLB.WIN_H -1.0f;

        float w = (ch.size_w * scale)/GLB.WIN_W;
        float h = (ch.size_h * scale)/GLB.WIN_H;

        mat4 projection(w, 0, 0, xpos,
                        0, h, 0, ypos,
                        0, 0, 1, 0,
                        0, 0, 0, 1);
        text_shader_.send_uniform("m4_projection"_h, projection);


        GFX::bind_texture2D(0, ch.tex_ID);
        buffer_unit_.draw(2, 0);

        // Advance to next glyph
        x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    vertex_array_.unbind();
    text_shader_.unuse();
    GFX::unbind_texture2D();
    GFX::disable_blending();
}

void TextRenderer::render(Scene* pscene)
{
    while (!line_queue_.empty())
    {
        const LineInfo& line = line_queue_.front();
        set_face(line.face);
        render_line(line.text, line.x, line.y, line.scale, line.color);
        line_queue_.pop();
    }
}

}
