#ifndef SHADOW_MAP_RENDERER_H
#define SHADOW_MAP_RENDERER_H

#include "renderer.h"
#include "shader.h"

#ifdef __EXPERIMENTAL_VSM_BLUR__
    #include "ping_pong_buffer.h"
#endif

namespace wcore
{

class ShadowMapRenderer : public Renderer
{
public:
    explicit ShadowMapRenderer();
    virtual ~ShadowMapRenderer();

    inline const math::mat4& get_light_matrix() const { return light_matrix_; }
    inline float get_normal_offset() const            { return normal_offset_; }
    inline float& get_normal_offset()                 { return normal_offset_; }
    inline void set_normal_offset(float value)        { normal_offset_ = value; }

    virtual void render(Scene* pscene) override;

    static math::vec2 SHADOW_TEXEL_SIZE;

private:
    Shader sm_shader_;
#ifdef __EXPERIMENTAL_VSM_BLUR__
    PingPongBuffer ping_pong_;
#endif

    float normal_offset_;
    math::mat4 light_matrix_; // Last light view-projection matrix

    static uint32_t SHADOW_WIDTH;
    static uint32_t SHADOW_HEIGHT;

};

}

#endif // SHADOW_MAP_RENDERER_H
