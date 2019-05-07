#ifndef PROCESSING_STAGE_H
#define PROCESSING_STAGE_H
#include <memory>
#include <functional>

#include "frame_buffer.h"
#include "wtypes.h"

namespace wcore
{

class BufferModule
{
protected:
    std::shared_ptr<Texture> out_texture_;
    FrameBuffer              frame_buffer_; // FBO, attached to output texture
    uint32_t width_;  // FBO and output texture width
    uint32_t height_; // FBO and output texture height

public:
    BufferModule(const char* out_tex_name,
                 std::shared_ptr<Texture> ptexture,
                 std::vector<GLenum>&& attachments,
                 bool register_as_global=false);

    virtual ~BufferModule(){}

    inline void bind_as_target();
    inline void unbind_as_target();
    void bind_as_source();
    void bind_as_source(uint32_t unit, uint32_t index);
    void unbind_as_source();
    void rebind_draw_buffers();

    inline uint32_t get_width() { return width_; }
    inline uint32_t get_height() { return height_; }
    uint32_t get_num_textures() const;
    FrameBuffer& get_frame_buffer() { return frame_buffer_; }

    void generate_mipmaps(uint32_t unit,
                          uint32_t base_level = 0,
                          uint32_t max_level = 3) const;

    inline void draw_to(std::function<void(void)> doFunc);
    inline void blit_depth(BufferModule& destination);
    inline void blit_depth_to_screen(uint32_t scrWidth, uint32_t scrHeight);

    inline Texture& get_texture() { return *out_texture_; }

    GLuint operator[](uint8_t index) const;
};

inline void BufferModule::bind_as_target()
{
    frame_buffer_.bind_as_render_target();
}

inline void BufferModule::unbind_as_target()
{
    frame_buffer_.unbind();
}

inline void BufferModule::draw_to(std::function<void(void)> doFunc)
{
    frame_buffer_.with_render_target(doFunc);
}

inline void BufferModule::blit_depth(BufferModule& destination)
{
    frame_buffer_.blit_depth(destination.frame_buffer_);
}

inline void BufferModule::blit_depth_to_screen(uint32_t scrWidth, uint32_t scrHeight)
{
    frame_buffer_.blit_depth_default_fb(scrWidth, scrHeight);
}

}

#endif // PROCESSING_STAGE_H
