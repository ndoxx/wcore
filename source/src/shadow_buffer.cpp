#include <GL/glew.h>
#include "shadow_buffer.h"
#include "texture.h"

ShadowBuffer::ShadowBuffer(unsigned int width,
                           unsigned int height):
BufferModule("shadowmap",
std::make_shared<Texture>(
    std::vector<hash_t>{H_("shadowTex")},
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
    std::vector<GLenum>     {GL_LINEAR},
    std::vector<GLenum>     {GL_RGBA32F},
    std::vector<GLenum>     {GL_RGBA},
#else
    std::vector<GLenum>     {GL_NEAREST},
    std::vector<GLenum>     {GL_DEPTH_COMPONENT24},
    std::vector<GLenum>     {GL_DEPTH_COMPONENT},
#endif
    width,
    height,
    GL_TEXTURE_2D,
    true),
#ifdef __EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__
{GL_COLOR_ATTACHMENT0}
#else
{GL_DEPTH_ATTACHMENT}
#endif
){}

ShadowBuffer::~ShadowBuffer() = default;
