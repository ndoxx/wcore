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

    // * Framebuffer
    // TODO: return a handle instead
    // Get an index to default framebuffer
    virtual uint32_t get_default_framebuffer() = 0;
    // Set the default framebuffer
    virtual void set_default_framebuffer(uint32_t index) = 0;
    // Bind the default framebuffer
    virtual void bind_default_frame_buffer() = 0;
    // Read framebuffer content to an array
    virtual void read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels) = 0;

    // * Draw commands
    // Draw a given number of primitives using currently bound index buffer, starting at a given offset
    virtual void draw_indexed(DrawPrimitive primitive, uint32_t n_elements, uint32_t offset) = 0;
    // Set the color used to clear any framebuffer
    virtual void set_clear_color(float r, float g, float b, float a) = 0;
    // Clear currently bound framebuffer
    virtual void clear(int flags) = 0;
    // Prevent from drawing in current framebuffer's color attachment(s)
    virtual void lock_color_buffer() = 0;

    // * Depth-stencil state
    // Lock/Unlock writing to the current framebuffer's depth buffer
    virtual void set_depth_lock(bool value) = 0;
    // Lock/Unlock writing to the current framebuffer's stencil
    virtual void set_stencil_lock(bool value) = 0;
    // Set function used as a depth test
    virtual void set_depth_func(DepthFunc value) = 0;
    // Enable/Disable depth test
    virtual void set_depth_test_enabled(bool value) = 0;
    // Set function used as a stencil test, with a reference value and a mask
    virtual void set_stencil_func(StencilFunc value, uint16_t a=0, uint16_t b=0) = 0;
    // Specify front/back stencil test action
    virtual void set_stencil_operator(StencilOperator value) = 0;
    // Enable/Disable stencil test
    virtual void set_stencil_test_enabled(bool value) = 0;

    // * Blending
    // Enable blending with blending equation dst.rgb = src.a*src.rgb + (1-src.a)*dst.rgb
    virtual void set_std_blending() = 0;
    // Enable blending with blending equation dst.rgb = src.rgb + dst.rgb
    virtual void set_light_blending() = 0;
    // Disable blending
    virtual void disable_blending() = 0;

    // * Byte-alignment
    // Specify alignment constraint on pixel rows when data is fed to client (value must be in {1,2,4,8})
    virtual void set_pack_alignment(uint32_t value) = 0;
    // Specify alignment constraint on pixel rows when data is read from client (value must be in {1,2,4,8})
    virtual void set_unpack_alignment(uint32_t value) = 0;

    // * Raster state
    // Set the position and size of area to draw to
    virtual void viewport(float xx, float yy, float width, float height) = 0;
    // Set which faces to cull out (front/back/none)
    virtual void set_cull_mode(CullMode value) = 0;
    // Set the line width for next line primitive draw calls
    virtual void set_line_width(float value) = 0;

    // * Sync
    // Wait till all graphics commands have been processed by device
    virtual void finish() = 0;
    // Force issued commands to yield in a finite time
    virtual void flush() = 0;

    // * Debug
    // Get current error from graphics device
    virtual uint32_t get_error() = 0;
    // Fail on graphics device error
    virtual void assert_no_error() = 0;

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
