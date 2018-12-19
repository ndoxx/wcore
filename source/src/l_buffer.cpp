#include <GL/glew.h>
#include "l_buffer.h"
#include "texture.h"

namespace wcore
{

LBuffer::LBuffer(unsigned int screenWidth,
                 unsigned int screenHeight):
BufferModule("lbuffer",
std::make_shared<Texture>(
    std::vector<hash_t>{H_("screenTex"), H_("brightTex"),         H_("ldepthStencilTex")},
    std::vector<GLenum> {GL_NEAREST,     GL_LINEAR_MIPMAP_LINEAR, GL_NONE},
    std::vector<GLenum> {GL_RGB16F,      GL_RGB,                  GL_DEPTH24_STENCIL8},
    std::vector<GLenum> {GL_RGB,         GL_RGB,                  GL_DEPTH_STENCIL},
    screenWidth,            // brightTex will contain the bright map.
    screenHeight,           // We use the multiple render target scheme
    GL_TEXTURE_2D,          // to populate this texture during the
    true,                   // lighting pass.
    true), // Lazy mipmap initialization needed
{GL_COLOR_ATTACHMENT0,
 GL_COLOR_ATTACHMENT1,
 GL_DEPTH_STENCIL_ATTACHMENT
}){}

LBuffer::~LBuffer() = default;

}
