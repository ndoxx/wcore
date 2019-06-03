#ifndef GFX_DRIVER_H
#define GFX_DRIVER_H

#include <cstdint>

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

class RendererAPI
{
public:
    virtual ~RendererAPI();

    virtual void viewport(float xx, float yy, float width, float height) = 0;

    virtual uint32_t get_default_framebuffer() = 0;
    virtual void set_default_framebuffer(uint32_t index) = 0;
    virtual void bind_default_frame_buffer() = 0;

    virtual void draw_indexed(DrawPrimitive primitive, uint32_t n_elements, uint32_t offset) = 0;

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
    virtual void set_stencil_func(StencilFunc value, uint16_t a, uint16_t b) = 0;
    virtual void set_stencil_operator(StencilOperator value) = 0;
    virtual void set_stencil_test_enabled(bool value) = 0;

    virtual void finish() = 0;
    virtual void flush() = 0;

    virtual uint32_t get_error() = 0;
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

    inline static void viewport(float xx, float yy, float width, float height) { renderer_->viewport(xx, yy, width, height); }

    inline static uint32_t get_default_framebuffer() { return renderer_->get_default_framebuffer(); }
    inline static void set_default_framebuffer(uint32_t index) { renderer_->set_default_framebuffer(index); }
    inline static void bind_default_frame_buffer() { renderer_->bind_default_frame_buffer(); }

    inline static void draw_indexed(DrawPrimitive primitive, uint32_t n_elements, uint32_t offset) { renderer_->draw_indexed(primitive, n_elements, offset); }
    inline static void set_clear_color(float r, float g, float b, float a) { renderer_->set_clear_color(r,g,b,a); }
    inline static void clear(int flags) { renderer_->clear(flags); }
    inline static void lock_color_buffer() { renderer_->lock_color_buffer(); }
    inline static void set_depth_lock(bool value) { renderer_->set_depth_lock(value); }
    inline static void set_stencil_lock(bool value) { renderer_->set_stencil_lock(value); }
    inline static void set_cull_mode(CullMode value) { renderer_->set_cull_mode(value); }

    inline static void set_std_blending() { renderer_->set_std_blending(); }
    inline static void set_light_blending() { renderer_->set_light_blending(); }
    inline static void disable_blending() { renderer_->disable_blending(); }

    inline static void set_depth_func(DepthFunc value) { renderer_->set_depth_func(value); }
    inline static void set_depth_test_enabled(bool value) { renderer_->set_depth_test_enabled(value); }
    inline static void set_stencil_func(StencilFunc value, uint16_t a=0, uint16_t b=0) { renderer_->set_stencil_func(value, a, b); }
    inline static void set_stencil_operator(StencilOperator value) { renderer_->set_stencil_operator(value); }
    inline static void set_stencil_test_enabled(bool value) { renderer_->set_stencil_test_enabled(value); }

    inline static void finish() { renderer_->finish(); }
    inline static void flush() { renderer_->flush(); }

    inline static uint32_t get_error() { return renderer_->get_error(); }
    inline static void assert_no_error() { renderer_->assert_no_error(); }

    // TMP
    inline static void bind_texture2D(uint32_t unit, uint32_t tex_handle) { renderer_->bind_texture2D(unit, tex_handle); }
    inline static void unbind_texture2D() { renderer_->unbind_texture2D(); }

private:
    static GfxAPI api_;
    static RendererAPI* renderer_;
};


} // namespace wcore

#endif // GFX_DRIVER_H
