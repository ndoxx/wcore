#include "ft2build.h"
#include FT_FREETYPE_H

#include "text_renderer.h"
#include "gfx_api.h"
#include "texture.h"
#include "logger.h"
#include "vertex_format.h"
#include "mesh.hpp"
#include "globals.h"
#include "file_system.h"
#include "geometry_common.h"
#include "error.h"

namespace wcore
{

using namespace math;

struct Character
{
    uint32_t tex_ID;  // ID handle of the glyph texture
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

struct TextRenderer::FontLibImpl
{
    FontLibImpl();
    ~FontLibImpl();

    FT_Library ft_;
    std::unordered_map<hash_t, FT_Face> faces_;
    std::unordered_map<hash_t, std::map<char, Character>> charsets_;
    std::unordered_map<hash_t, std::vector<std::unique_ptr<Texture>>> textures_;
    std::queue<LineInfo> line_queue_;
};

TextRenderer::FontLibImpl::FontLibImpl():
ft_()
{
    if (FT_Init_FreeType(&ft_))
    {
        DLOGF("[TextRenderer] Could not init FreeType Library.", "text");
        fatal("Could not init FreeType Library.");
    }
}

TextRenderer::FontLibImpl::~FontLibImpl()
{
    for(auto p : faces_)
        FT_Done_Face(p.second);
    FT_Done_FreeType(ft_);
}



TextRenderer::TextRenderer():
pimpl_(new FontLibImpl()),
text_shader_(ShaderResource("text.vert;text.frag"))
{

}

TextRenderer::~TextRenderer()
{

}

void TextRenderer::load_face(const char* fontname,
                             uint32_t height,
                             uint32_t width)
{
    // Get stream to file and load ttf face to memory
    hash_t hname(H_(fontname));
    std::string font_file(fontname);
    font_file += ".ttf";

    auto pstream = FILESYSTEM.get_file_as_stream(font_file.c_str(), "root.folders.font"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGE("[TextRenderer] Cannot get stream to file: ", "text");
        DLOGI(font_file, "text");
        return;
    }
    std::vector<char> buffer((std::istreambuf_iterator<char>(*pstream)), std::istreambuf_iterator<char>());
    FT_Face face;
    if(FT_New_Memory_Face(pimpl_->ft_, reinterpret_cast<FT_Byte*>(&buffer[0]), buffer.size(), 0, &face))
    {
        DLOGE("[TextRenderer] Failed to load font: <p>" + font_file + "</p>", "text");
        return;
    }

    // Set face size
    FT_Set_Pixel_Sizes(face, width, height);
    // Disable byte-alignment restriction
    Gfx::device->set_unpack_alignment(1);

    // Generate character glyphs
    std::map<char, Character> characters;
    for (unsigned char cc = 0; cc < 128; ++cc)
    {
        // Load character glyph
        if (FT_Load_Char(face, cc, FT_LOAD_RENDER))
        {
            DLOGW(std::string("[TextRenderer] Failed to load Glyph: \'") + std::to_string(cc) + "\'", "text");
            continue;
        }
        // Generate texture
        pimpl_->textures_[hname].push_back(std::make_unique<Texture>
        (
            std::initializer_list<TextureUnitInfo>
            {
                TextureUnitInfo("charTex"_h,
                                TextureFilter(TextureFilter::MIN_LINEAR | TextureFilter::MAG_LINEAR),
                                TextureIF::R8,
                                face->glyph->bitmap.buffer)
            },
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            TextureWrap::CLAMP_TO_EDGE
        ));
        uint32_t index = pimpl_->textures_[hname].size()-1;

        Character character =
        {
            index,
            face->glyph->advance.x,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            face->glyph->bitmap_left,
            face->glyph->bitmap_top,

        };
        characters.insert(std::pair<char, Character>(cc, character));
    }

    pimpl_->faces_.insert(std::make_pair(hname, face));
    pimpl_->charsets_.insert(std::make_pair(hname, characters));

    set_face(hname);

#ifdef __DEBUG__
    DLOGN("[TextRenderer] New face: <n>" + std::string(fontname) + "</n>", "text");
    DLOGI("from file: <p>" + font_file + "</p>", "text");
#endif

    // Restore byte-alignment state
    Gfx::device->set_unpack_alignment(4);
}

void TextRenderer::render_line(const std::string& text, float x, float y, float scale, math::vec3 color)
{
    x *= 2;
    y *= 2;
    Gfx::device->bind_default_frame_buffer();
    Gfx::device->viewport(0,0,GLB.WIN_W,GLB.WIN_H);
    text_shader_.use();
    text_shader_.send_uniform("v3_textColor"_h, color);

    Gfx::device->set_std_blending();
    // Iterate through all characters
    std::string::const_iterator itc;
    for (itc = text.begin(); itc != text.end(); ++itc)
    {
        const Character& ch = pimpl_->charsets_[current_face_][char(*itc)];

        float xpos = (x + ch.bearing_x * scale)/GLB.WIN_W -1.0f;
        float ypos = (y - (ch.size_h - ch.bearing_y) * scale)/GLB.WIN_H -1.0f;

        float w = (ch.size_w * scale)/GLB.WIN_W;
        float h = (ch.size_h * scale)/GLB.WIN_H;

        mat4 projection(w, 0, 0, xpos,
                        0, h, 0, ypos,
                        0, 0, 1, 0,
                        0, 0, 0, 1);
        text_shader_.send_uniform("m4_projection"_h, projection);


        //Gfx::device->bind_texture2D(0, ch.tex_ID);
        pimpl_->textures_[current_face_][ch.tex_ID]->bind(0,0);
        CGEOM.draw("char_quad"_h);

        // Advance to next glyph
        x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    text_shader_.unuse();
    Gfx::device->disable_blending();
}

void TextRenderer::schedule_for_drawing(const std::string& text,
                                        hash_t face,
                                        float x,
                                        float y,
                                        float scale,
                                        math::vec3 color)
{
    pimpl_->line_queue_.push({text, face, x, y, scale, color});
}

void TextRenderer::render(Scene* pscene)
{
    while (!pimpl_->line_queue_.empty())
    {
        const LineInfo& line = pimpl_->line_queue_.front();
        set_face(line.face);
        render_line(line.text, line.x, line.y, line.scale, line.color);
        pimpl_->line_queue_.pop();
    }
}

}
