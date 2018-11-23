#ifndef SHADOW_MAP_RENDERER_H
#define SHADOW_MAP_RENDERER_H

#include "renderer.hpp"
#include "shader.h"
#include "frame_buffer.h"

namespace wcore
{

class ShadowBuffer;

class ShadowMapRenderer : public Renderer<Vertex3P>
{
private:
    ShadowBuffer* sbuffer_;
    Shader sm_shader_;
#ifdef __EXPERIMENTAL_VSM_BLUR__
    Shader blur_pass_shader_;
    std::shared_ptr<Texture> tmp_tex_;
    FrameBuffer tmp_fbo_;
#endif

    static uint32_t SHADOW_WIDTH;
    static uint32_t SHADOW_HEIGHT;

public:
    explicit ShadowMapRenderer();
    virtual ~ShadowMapRenderer();

    virtual void render() override {}
    math::mat4 render_directional_shadow_map();

    static math::vec2 SHADOW_TEXEL_SIZE;
};

}

#endif // SHADOW_MAP_RENDERER_H
