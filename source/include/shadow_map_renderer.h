#ifndef SHADOW_MAP_RENDERER_H
#define SHADOW_MAP_RENDERER_H

#include "renderer.hpp"
#include "shader.h"

#ifdef __EXPERIMENTAL_VSM_BLUR__
    #include "ping_pong_buffer.h"
#endif

namespace wcore
{

class ShadowBuffer;

class ShadowMapRenderer : public Renderer<Vertex3P>
{
private:
    Shader sm_shader_;
#ifdef __EXPERIMENTAL_VSM_BLUR__
    PingPongBuffer ping_pong_;
#endif
    ShadowBuffer* sbuffer_;

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
