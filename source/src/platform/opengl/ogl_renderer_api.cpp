#include <GL/glew.h>
#include <cassert>
#include <map>
#include "platform/opengl/ogl_renderer_api.h"

namespace wcore
{

static std::map<DrawPrimitive, GLenum> OGLPrimitive =
{
    {DrawPrimitive::Lines, GL_LINES},
    {DrawPrimitive::Triangles, GL_TRIANGLES},
    {DrawPrimitive::Quads, GL_QUADS}
};

OGLRenderDevice::OGLRenderDevice():
default_framebuffer_(0)
{

}

OGLRenderDevice::~OGLRenderDevice()
{

}

void OGLRenderDevice::viewport(float xx, float yy, float width, float height)
{
    glViewport(xx, yy, width, height);
}

uint32_t OGLRenderDevice::get_default_framebuffer()
{
    return default_framebuffer_;
}

void OGLRenderDevice::set_default_framebuffer(uint32_t index)
{
    default_framebuffer_ = index;
}

void OGLRenderDevice::bind_default_frame_buffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, default_framebuffer_);
}


void OGLRenderDevice::draw_indexed(DrawPrimitive primitive, uint32_t n_elements, uint32_t offset)
{
    glDrawElements(OGLPrimitive[primitive],
                   uint32_t(primitive)*n_elements,
                   GL_UNSIGNED_INT,
                   (void*)(offset * sizeof(GLuint)));
}

void OGLRenderDevice::read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels)
{
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
}

void OGLRenderDevice::set_clear_color(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void OGLRenderDevice::clear(int flags)
{
    GLenum clear_bits = 0;
    clear_bits |= (flags & CLEAR_COLOR_FLAG) ? GL_COLOR_BUFFER_BIT : 0;
    clear_bits |= (flags & CLEAR_DEPTH_FLAG) ? GL_DEPTH_BUFFER_BIT : 0;
    clear_bits |= (flags & CLEAR_STENCIL_FLAG) ? GL_STENCIL_BUFFER_BIT : 0;
    glClear(clear_bits);
}

void OGLRenderDevice::lock_color_buffer()
{
    glDrawBuffer(0);
}

void OGLRenderDevice::set_depth_lock(bool value)
{
    glDepthMask(!value);
}

void OGLRenderDevice::set_stencil_lock(bool value)
{
    glStencilMask(!value);
}

void OGLRenderDevice::set_cull_mode(CullMode value)
{
    switch(value)
    {
        case CullMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
    }
}

void OGLRenderDevice::set_std_blending()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OGLRenderDevice::set_light_blending()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
}

void OGLRenderDevice::disable_blending()
{
    glDisable(GL_BLEND);
}

void OGLRenderDevice::set_depth_func(DepthFunc value)
{
    switch(value)
    {
        case DepthFunc::Less:
            glDepthFunc(GL_LESS);
            break;

        case DepthFunc::LEqual:
            glDepthFunc(GL_LEQUAL);
            break;
    }
}

void OGLRenderDevice::set_depth_test_enabled(bool value)
{
    if(value)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void OGLRenderDevice::set_stencil_func(StencilFunc value, uint16_t a, uint16_t b)
{
    switch(value)
    {
        case StencilFunc::Always:
            glStencilFunc(GL_ALWAYS, 0, 0);
            break;

        case StencilFunc::NotEqual:
            glStencilFunc(GL_NOTEQUAL, a, b);
            break;
    }
}

void OGLRenderDevice::set_stencil_operator(StencilOperator value)
{
    if(value == StencilOperator::LightVolume)
    {
#ifndef __OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__
        // If depth test fails, back faces polygons increment the stencil value
        // else nothing changes
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
        // If depth test fails, front faces polygons decrement the stencil value
        // else nothing changes
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
#else
        // If depth test fails, front and back polygons invert the stencil value bitwise
        // else nothing changes
        glStencilOp(GL_KEEP, GL_INVERT, GL_KEEP);
#endif //__OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__
    }
}

void OGLRenderDevice::set_stencil_test_enabled(bool value)
{
    if(value)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
}

void OGLRenderDevice::finish()
{
    glFinish();
}

void OGLRenderDevice::flush()
{
    glFlush();
}

uint32_t OGLRenderDevice::get_error()
{
    return glGetError();
}

void OGLRenderDevice::assert_no_error()
{
    assert(glGetError()==0);
}

void OGLRenderDevice::set_line_width(float value)
{
    glLineWidth(value);
}

void OGLRenderDevice::bind_texture2D(uint32_t unit, uint32_t tex_handle)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, tex_handle);
}

void OGLRenderDevice::unbind_texture2D()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace wcore
