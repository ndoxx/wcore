#include "SSAO_buffer.h"
#include "texture.h"

namespace wcore
{

SSAOBuffer::SSAOBuffer(unsigned int width,
                       unsigned int height):
BufferModule("SSAObuffer",
std::make_shared<Texture>(
    std::vector<hash_t>{"SSAOTex"_h},
    std::vector<GLenum>{GL_LINEAR},
    std::vector<GLenum>{GL_R8},
    std::vector<GLenum>{GL_RED},
    width,
    height,
    GL_TEXTURE_2D,
    true),
{GL_COLOR_ATTACHMENT0}){}

SSAOBuffer::~SSAOBuffer() = default;

}
