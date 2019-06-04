#include "buffer_module.h"
#include "texture.h"
#include "mesh_factory.h"

namespace wcore
{

std::map<hash_t, std::unique_ptr<BufferModule>> GMODULES::modules_;


BufferModule::BufferModule(const char* out_tex_name,
                           std::unique_ptr<Texture> ptexture):
out_texture_(std::move(ptexture)),
frame_buffer_(*out_texture_),
width_(out_texture_->get_width()),
height_(out_texture_->get_height()),
name_(H_(out_tex_name))
{
#ifdef __DEBUG__
    HRESOLVE.add_intern_string(out_tex_name);
#endif
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
    return out_texture_->get_num_units();
}

void BufferModule::generate_mipmaps(uint32_t unit,
                                    uint32_t base_level,
                                    uint32_t max_level) const
{
    out_texture_->generate_mipmaps(unit, base_level, max_level);
}

uint32_t BufferModule::operator[](uint8_t index) const
{
    return (*out_texture_)[index];
}

}
