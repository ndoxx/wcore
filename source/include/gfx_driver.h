#ifndef GFX_DRIVER_H
#define GFX_DRIVER_H

#include <GL/glew.h>
#include <cassert>
#include "math3d.h"

namespace wcore
{
namespace GFX
{
static unsigned int DEFAULT_FRAMEBUFFER = 0;

inline uint32_t get_error()         { return glGetError(); }
inline void assert_no_error()       { assert(glGetError()==0); }

inline void set_std_blending()      { glBlendEquation(GL_FUNC_ADD); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); }
inline void set_light_blending()    { glBlendEquation(GL_FUNC_ADD); glBlendFunc(GL_ONE, GL_ONE); }
inline void set_depth_test_less()   { glDepthFunc(GL_LESS); }
inline void enable_blending()       { glEnable(GL_BLEND); }
inline void disable_blending()      { glDisable(GL_BLEND); }
inline void enable_stencil()        { glEnable(GL_STENCIL_TEST); }
inline void disable_stencil()       { glDisable(GL_STENCIL_TEST); }
inline void enable_depth_testing()  { glEnable(GL_DEPTH_TEST); }
inline void disable_depth_testing() { glDisable(GL_DEPTH_TEST); }
inline void lock_depth_buffer()     { glDepthMask(GL_FALSE); }
inline void unlock_depth_buffer()   { glDepthMask(GL_TRUE); }
inline void lock_stencil()          { glStencilMask(GL_FALSE); }
inline void unlock_stencil()        { glStencilMask(GL_TRUE); }
inline void cull_front()            { glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); }
inline void cull_back()             { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); }
inline void disable_face_culling()  { glDisable(GL_CULL_FACE); }
inline void clear_color_depth()     { glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); }
inline void clear_color()           { glClear(GL_COLOR_BUFFER_BIT); }
inline void clear_depth()           { glClear(GL_DEPTH_BUFFER_BIT); }
inline void clear_stencil()         { glClear(GL_STENCIL_BUFFER_BIT); }

inline void set_stencil_always_pass() { glStencilFunc(GL_ALWAYS, 0, 0); }
inline void set_stencil_notequal(uint16_t a, uint16_t b)    { glStencilFunc(GL_NOTEQUAL, a, b); }
inline void set_stencil_op_light_volume()
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

inline void lock_color_buffer()     { glDrawBuffer(0); }
inline void set_clear_color(const math:: vec4& color) { glClearColor(color.x(),color.y(),color.z(),color.w()); }

inline void bind_texture2D(uint32_t unit, uint32_t tex_index)
{
    glActiveTexture(GL_TEXTURE0+unit);
    glBindTexture(GL_TEXTURE_2D, tex_index);
}
inline void unbind_texture2D()          { glBindTexture(GL_TEXTURE_2D, 0); }
inline void unbind_vertex_array()       { glBindVertexArray(0); }
inline void bind_default_frame_buffer() { glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER); }

inline void set_default_framebuffer(unsigned int index) { DEFAULT_FRAMEBUFFER = index; }

inline void viewport(float xx, float yy, float width, float height)
{
    glViewport(xx,yy,width,height);
}

inline void finish() { glFinish(); }
inline void flush()  { glFlush(); }

} // namespace GFX
} // namespace wcore

#endif // GFX_DRIVER_H
