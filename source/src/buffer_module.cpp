#include "buffer_module.h"
#include "texture.h"
#include "mesh_factory.h"

namespace wcore
{

BufferModule::BufferModule(const char* out_tex_name,
                           std::shared_ptr<Texture> ptexture,
                           std::vector<GLenum>&& attachments):
out_texture_(ptexture),
frame_buffer_(*out_texture_, attachments),
width_(ptexture->get_width()),
height_(ptexture->get_height())
{
    // Register as a named texture
    Texture::register_named_texture(H_(out_tex_name), out_texture_);
}

BufferModule::BufferModule(unsigned int width,
                           unsigned int height):
BufferModule("unused",
             std::make_shared<Texture>(std::vector<hash_t>{H_("unused")},
                                       width,
                                       height),
             std::vector<GLenum>{GL_NONE})
{

}

void BufferModule::rebind_draw_buffers()
{
    frame_buffer_.rebind_draw_buffers();
}

void BufferModule::bind_as_source()
{
    out_texture_->bind_all();
}

void BufferModule::bind_as_source(uint32_t unit, uint32_t index)
{
    out_texture_->bind(unit, index);
}

void BufferModule::unbind_as_source()
{
    out_texture_->unbind();
}

uint32_t BufferModule::get_num_textures() const
{
    return out_texture_->get_num_textures();
}

void BufferModule::generate_mipmaps(uint32_t unit,
                                    uint32_t base_level,
                                    uint32_t max_level) const
{
    out_texture_->generate_mipmaps(unit, base_level, max_level);
}

GLuint BufferModule::operator[](uint8_t index) const
{
    return (*out_texture_)[index];
}

}
