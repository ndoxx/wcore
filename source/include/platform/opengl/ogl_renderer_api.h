#ifndef OGL_RENDERER_API_H
#define OGL_RENDERER_API_H

#include "gfx_api.h"

namespace wcore
{

class OGLRenderDevice: public RenderDevice
{
public:
    OGLRenderDevice();
    virtual ~OGLRenderDevice();

    // * Framebuffer
    // TODO: return a handle instead
    // Get an index to default framebuffer
    virtual uint32_t get_default_framebuffer() override;
    // Set the default framebuffer
    virtual void set_default_framebuffer(uint32_t index) override;
    // Bind the default framebuffer
    virtual void bind_default_frame_buffer() override;
    // Read framebuffer content to an array
    virtual void read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels) override;

    // * Draw commands
    // Draw a given number of primitives using currently bound index buffer, starting at a given offset
    virtual void draw_indexed(DrawPrimitive primitive, uint32_t n_elements, uint32_t offset) override;
    // Set the color used to clear any framebuffer
    virtual void set_clear_color(float r, float g, float b, float a) override;
    // Clear currently bound framebuffer
    virtual void clear(int flags) override;
    // Prevent from drawing in current framebuffer's color attachment(s)
    virtual void lock_color_buffer() override;

    // * Depth-stencil state
    // Lock/Unlock writing to the current framebuffer's depth buffer
    virtual void set_depth_lock(bool value) override;
    // Lock/Unlock writing to the current framebuffer's stencil
    virtual void set_stencil_lock(bool value) override;
    // Set function used as a depth test
    virtual void set_depth_func(DepthFunc value) override;
    // Enable/Disable depth test
    virtual void set_depth_test_enabled(bool value) override;
    // Set function used as a stencil test, with a reference value and a mask
    virtual void set_stencil_func(StencilFunc value, uint16_t a=0, uint16_t b=0) override;
    // Specify front/back stencil test action
    virtual void set_stencil_operator(StencilOperator value) override;
    // Enable/Disable stencil test
    virtual void set_stencil_test_enabled(bool value) override;

    // * Blending
    // Enable blending with blending equation dst.rgb = src.a*src.rgb + (1-src.a)*dst.rgb
    virtual void set_std_blending() override;
    // Enable blending with blending equation dst.rgb = src.rgb + dst.rgb
    virtual void set_light_blending() override;
    // Disable blending
    virtual void disable_blending() override;

    // * Byte-alignment
    // Specify alignment constraint on pixel rows when data is fed to client (value must be in {1,2,4,8})
    virtual void set_pack_alignment(uint32_t value) override;
    // Specify alignment constraint on pixel rows when data is read from client (value must be in {1,2,4,8})
    virtual void set_unpack_alignment(uint32_t value) override;

    // * Raster state
    // Set the position and size of area to draw to
    virtual void viewport(float xx, float yy, float width, float height) override;
    // Set which faces to cull out (front/back/none)
    virtual void set_cull_mode(CullMode value) override;
    // Set the line width for next line primitive draw calls
    virtual void set_line_width(float value) override;

    // * Sync
    // Wait till all graphics commands have been processed by device
    virtual void finish() override;
    // Force issued commands to yield in a finite time
    virtual void flush() override;

    // * Debug
    // Get current error from graphics device
    virtual uint32_t get_error() override;
    // Fail on graphics device error
    virtual void assert_no_error() override;

    // TMP
    virtual void bind_texture2D(uint32_t unit, uint32_t tex_handle) override;
    virtual void unbind_texture2D() override;

private:
    uint32_t default_framebuffer_;
};

} // namespace wcore

#endif
