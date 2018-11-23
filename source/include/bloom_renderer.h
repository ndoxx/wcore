#ifndef BLUR_PASS_H
#define BLUR_PASS_H

#include <vector>

#include "renderer.hpp"
#include "shader.h"
#include "frame_buffer.h"

namespace wcore
{

class Texture;
class BloomRenderer : public Renderer<Vertex3P>
{
private:
    Shader blur_pass_shader_;

    std::vector<std::shared_ptr<Texture>> bloom_h_tex_;
    std::vector<std::shared_ptr<Texture>> bloom_tex_;
    std::vector<FrameBuffer*> fbo_h_;
    FrameBuffer* fbo_;

public:
    BloomRenderer();
    virtual ~BloomRenderer();
    virtual void render() override;
};

}

#endif // BLUR_PASS_H
