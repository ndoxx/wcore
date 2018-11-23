#include "SSAO_buffer.h"
#include "texture.h"

namespace wcore
{

SSAOBuffer::SSAOBuffer(unsigned int width,
                       unsigned int height):
BufferModule("SSAObuffer",
std::make_shared<Texture>(
    std::vector<hash_t>{H_("SSAOTex")},
    std::vector<GLenum>{GL_NEAREST},
    std::vector<GLenum>{GL_RED},
    std::vector<GLenum>{GL_RGB},
    width,
    height,
    GL_TEXTURE_2D,
    true),
{GL_COLOR_ATTACHMENT0}){}

SSAOBuffer::~SSAOBuffer() = default;

}
