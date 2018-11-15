#include "g_buffer.h"
#include "texture.h"

/*
    G-Buffer element groups:
    * Normal, Metallic and AO
    are combined in an RGBA container (sampler name = normalTex)
        texture(normalTex, texCoord).rg -> Compressed normal (view space)
        texture(normalTex, texCoord).b  -> Material metallicity
        texture(normalTex, texCoord).a  -> Material ambient occlusion

    * Albedo (diffuse) and Roughness
    are combined in an RGBA container (sampler name = albedoTex)
        texture(albedoTex, texCoord).rgb -> Albedo color
        texture(albedoTex, texCoord).a   -> Material roughness
*/

GBuffer::GBuffer(unsigned int width,
                 unsigned int height):
BufferModule("gbuffer",
std::make_shared<Texture>(
    std::vector<hash_t>{H_("normalTex"), H_("albedoTex"), H_("depthTex")},
    std::vector<GLenum>{GL_NEAREST,      GL_NEAREST,      GL_NONE},
    std::vector<GLenum>{GL_RGBA16_SNORM, GL_RGBA16,       GL_DEPTH_COMPONENT24},
    std::vector<GLenum>{GL_RGBA,         GL_RGBA,         GL_DEPTH_COMPONENT},
    width,
    height,
    GL_TEXTURE_2D,
    true),
{GL_COLOR_ATTACHMENT0,
 GL_COLOR_ATTACHMENT1,
 GL_DEPTH_ATTACHMENT}){}

GBuffer::~GBuffer() = default;
