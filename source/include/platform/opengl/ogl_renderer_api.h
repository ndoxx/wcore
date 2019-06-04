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

    virtual void viewport(float xx, float yy, float width, float height) override;

    virtual uint32_t get_default_framebuffer() override;
    virtual void set_default_framebuffer(uint32_t index) override;
    virtual void bind_default_frame_buffer() override;

    virtual void draw_indexed(DrawPrimitive primitive, uint32_t n_elements, uint32_t offset) override;

    virtual void read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels) override;

    virtual void set_clear_color(float r, float g, float b, float a) override;
    virtual void clear(int flags) override;
    virtual void lock_color_buffer() override;
    virtual void set_depth_lock(bool value) override;
    virtual void set_stencil_lock(bool value) override;
    virtual void set_cull_mode(CullMode value) override;

    virtual void set_std_blending() override;
    virtual void set_light_blending() override;
    virtual void disable_blending() override;

    virtual void set_depth_func(DepthFunc value) override;
    virtual void set_depth_test_enabled(bool value) override;
    virtual void set_stencil_func(StencilFunc value, uint16_t a, uint16_t b) override;
    virtual void set_stencil_operator(StencilOperator value) override;
    virtual void set_stencil_test_enabled(bool value) override;

    virtual void finish() override;
    virtual void flush() override;

    virtual uint32_t get_error() override;
    virtual void assert_no_error() override;

    // Style
    virtual void set_line_width(float value) override;

    // TMP
    virtual void bind_texture2D(uint32_t unit, uint32_t tex_handle) override;
    virtual void unbind_texture2D() override;

private:
    uint32_t default_framebuffer_;
};

} // namespace wcore

#endif
