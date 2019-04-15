#include "SSR_buffer.h"
#include "texture.h"

namespace wcore
{

SSRBuffer::SSRBuffer(unsigned int width,
                     unsigned int height):
BufferModule("SSRbuffer",
std::make_shared<Texture>(
    std::vector<hash_t>{"SSRTex"_h},
    std::vector<GLenum>{GL_NEAREST},
    std::vector<GLenum>{GL_RGB16F},
    std::vector<GLenum>{GL_RGB},
    width,
    height,
    GL_TEXTURE_2D,
    true),
{GL_COLOR_ATTACHMENT0}){}

SSRBuffer::~SSRBuffer() = default;

} // namespace wcore
