#include <cassert>

#include "frame_buffer.h"
#include "texture.h"
#include "logger.h"

FrameBuffer::FrameBuffer(const Texture& texture, const std::vector<GLenum>& attachments):
frame_buffer_(0),
render_buffer_(0),
draw_buffers_(nullptr),
n_textures_(texture.get_num_textures()),
width_(texture.get_width()),
height_(texture.get_height())
{
    GLenum textureTarget = texture.get_texture_target();

    assert(attachments.size() == n_textures_);
    assert(n_textures_ <= 32); // Assert to be sure no buffer overrun should occur
    draw_buffers_ = new GLenum[n_textures_];

    #if __DEBUG_TEXTURE_VERBOSE__
        DLOGN("[FrameBuffer] Initializing as render target.");
        DLOGI("width:  <v>" + std::to_string(width_)  + "</v>");
        DLOGI("height: <v>" + std::to_string(height_) + "</v>");
    #endif

    /*
        If there is a depth attachment, it means we need to do z-testing
        in the next render pass. So we need to do sampling. So we need a
        texture. So we use the corresponding texture id.

        If there is no depth attachment, we use a render buffer object
        as a z-buffer.
    */

    bool hasDepth = false; // Do we have a depth attachment in the attachment array?
    // Check attachments
    for(int ii = 0; ii < n_textures_; ++ii)
    {
        // No attachment, continue to next texture id
        if(attachments[ii] == GL_NONE) continue;

        // Generate FBO
        if(frame_buffer_ == 0)
        {
            glGenFramebuffers(1, &frame_buffer_);
            glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);

            #if __DEBUG_TEXTURE_VERBOSE__
                DLOGI("Generated new FBO.");
            #endif
        }

        // Depth -> use texture
        if(attachments[ii] == GL_DEPTH_ATTACHMENT ||
           attachments[ii] == GL_DEPTH_STENCIL_ATTACHMENT)
        {
            draw_buffers_[ii] = GL_NONE;
            hasDepth = true;
        }
        else
            draw_buffers_[ii] = attachments[ii];

        // Attach texture to frame buffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachments[ii], textureTarget, texture[ii], 0);
    }

    // Each attachment was GL_NONE
    if(frame_buffer_ == 0)
    {
        delete [] draw_buffers_;
        draw_buffers_ = nullptr;
        return;
    }

    // No depth -> Attach a render buffer to frame buffer as a z-buffer
    if(!hasDepth)
    {
        glGenRenderbuffers(1, &render_buffer_);
        glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_);
    }

    if(hasDepth && n_textures_ == 1) // Depth only texture
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        // Specify list of color buffers to draw to
        glDrawBuffers(n_textures_, draw_buffers_);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        DLOGE("[Texture] Framebuffer creation failed!");
        switch(status)
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                DLOGI("Not all framebuffer attachment points are framebuffer attachment complete.");
                break;
            /*case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                DLOGI("Not all attached images have the same width and height.");
                break;*/
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                DLOGI("No images are attached to the framebuffer.");
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                DLOGI("The combination of internal formats of the attached images violates an implementation-dependent set of restrictions.");
                break;
        }
        throw std::runtime_error("Framebuffer creation <b>failed</b>!");
    }
    else
    {
        #if __DEBUG_TEXTURE_VERBOSE__
            DLOGI("Framebuffer creation <g>complete</g>.");
        #endif
        // Save texture indices
            for(uint32_t ii=0; ii<n_textures_; ++ii)
                texture_indices_.push_back(texture[ii]);
    }

    // Unbind frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
    if(frame_buffer_) glDeleteFramebuffers(1, &frame_buffer_);
    if(render_buffer_) glDeleteRenderbuffers(1, &render_buffer_);
    if(draw_buffers_) delete [] draw_buffers_;
}

void FrameBuffer::rebind_draw_buffers() const
{
    if(draw_buffers_)
    {
        //glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
        glDrawBuffers(n_textures_, draw_buffers_);
    }
}

void FrameBuffer::bind_as_render_target() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);

#ifdef __PROFILING_SET_1x1_VIEWPORT__
    glViewport(0, 0, 1, 1);
#else
    glViewport(0, 0, width_, height_);
#endif
}

void FrameBuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::with_render_target(std::function<void(void)> doFunc) const
{
    bind_as_render_target();
    doFunc();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::blit_depth(FrameBuffer& destination) const
{
    // write depth buffer to destination framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.frame_buffer_);
    glBlitFramebuffer(0,            // src x0
                      0,            // src y0
                      width_,       // src x1
                      height_,      // src y1
                      0,            // dst x0
                      0,            // dst y0
                      destination.width_,   // dst x1
                      destination.height_,  // dst y1
                      GL_DEPTH_BUFFER_BIT,  // mask
                      GL_NEAREST);  // filter
}

void FrameBuffer::blit_depth_default_fb(uint32_t screenWidth, uint32_t screenHeight)
{
    // write depth buffer to default framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0,            // src x0
                      0,            // src y0
                      width_,       // src x1
                      height_,      // src y1
                      0,            // dst x0
                      0,            // dst y0
                      screenWidth,  // dst x1
                      screenHeight, // dst y1
                      GL_DEPTH_BUFFER_BIT,  // mask
                      GL_NEAREST);  // filter
}
