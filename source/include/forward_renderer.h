#ifndef FORWARD_RENDERER_H
#define FORWARD_RENDERER_H

#include "renderer.h"
#include "shader.h"

namespace wcore
{

class GBuffer;
class LBuffer;
class Camera;
class ForwardRenderer : public Renderer
{
private:
    Shader forward_stage_shader_;
    Shader skybox_shader_;

public:
    ForwardRenderer();
    virtual ~ForwardRenderer() = default;
    virtual void render(Scene* pscene) override;
};

}

#endif // FORWARD_RENDERER_H
