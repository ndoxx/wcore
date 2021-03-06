#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <vector>
#include <functional>

namespace wcore
{

class Texture;
class FrameBuffer
{
private:
    uint32_t frame_buffer_;
    uint32_t render_buffer_;
    uint32_t* draw_buffers_;
    std::vector<uint32_t> texture_indices_;
    int n_textures_;
    int width_;
    int height_;

public:
    FrameBuffer(const Texture& texture);
    ~FrameBuffer();

    inline uint32_t get_target_index(uint32_t position) { return texture_indices_[position]; }

    void bind_as_render_target() const;
    void unbind() const;
    void with_render_target(std::function<void(void)> doFunc) const;
    void blit_depth(FrameBuffer& destination) const;
    void blit_depth_default_fb(uint32_t screenWidth, uint32_t screenHeight);
    void rebind_draw_buffers() const;
};

}

#endif // FRAME_BUFFER_H
