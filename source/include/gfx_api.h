#ifndef GFX_DRIVER_H
#define GFX_DRIVER_H

#include <cstdint>
#include <memory>

namespace wcore
{

enum class GfxAPI
{
    None = 0,
    OpenGL = 1
};

enum class DrawPrimitive
{
    Lines = 2,
    Triangles = 3,
    Quads = 4
};

enum class CullMode
{
    None = 0,
    Front = 1,
    Back = 2
};

enum class DepthFunc
{
    Less = 0,
    LEqual = 1
};

enum class StencilFunc
{
    Always = 0,
    NotEqual = 1
};

enum class StencilOperator
{
    LightVolume = 0
};

enum ClearFlags
{
    CLEAR_COLOR_FLAG = 1,
    CLEAR_DEPTH_FLAG = 2,
    CLEAR_STENCIL_FLAG = 4
};

// Following API is subject to future HEAVY changes
class RenderDevice
{
public:
    virtual ~RenderDevice();

    virtual void viewport(float xx, float yy, float width, float height) = 0;

    virtual uint32_t get_default_framebuffer() = 0;
    virtual void set_default_framebuffer(uint32_t index) = 0;
    virtual void bind_default_frame_buffer() = 0;

    virtual void draw_indexed(DrawPrimitive primitive, uint32_t n_elements, uint32_t offset) = 0;

    virtual void read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels) = 0;

    virtual void set_clear_color(float r, float g, float b, float a) = 0;
    virtual void clear(int flags) = 0;
    virtual void lock_color_buffer() = 0;
    virtual void set_depth_lock(bool value) = 0;
    virtual void set_stencil_lock(bool value) = 0;
    virtual void set_cull_mode(CullMode value) = 0;

    virtual void set_std_blending() = 0;
    virtual void set_light_blending() = 0;
    virtual void disable_blending() = 0;

    virtual void set_depth_func(DepthFunc value) = 0;
    virtual void set_depth_test_enabled(bool value) = 0;
    virtual void set_stencil_func(StencilFunc value, uint16_t a=0, uint16_t b=0) = 0;
    virtual void set_stencil_operator(StencilOperator value) = 0;
    virtual void set_stencil_test_enabled(bool value) = 0;

    virtual void finish() = 0;
    virtual void flush() = 0;

    virtual uint32_t get_error() = 0;
    virtual void assert_no_error() = 0;

    // Byte-alignment
    virtual void set_pack_alignment(uint32_t value) = 0;
    virtual void set_unpack_alignment(uint32_t value) = 0;

    // Style
    virtual void set_line_width(float value) = 0;

    // TMP
    virtual void bind_texture2D(uint32_t unit, uint32_t tex_handle) = 0;
    virtual void unbind_texture2D() = 0;

};

class Gfx
{
public:
    inline static GfxAPI get_api() { return api_; }
    static void set_api(GfxAPI api);

    static std::unique_ptr<RenderDevice> device;

private:
    static GfxAPI api_;
};


} // namespace wcore

#endif // GFX_DRIVER_H
