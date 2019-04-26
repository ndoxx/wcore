#ifndef BLUR_PASS_H
#define BLUR_PASS_H

#include <vector>

#include "renderer.hpp"
#include "shader.h"
#include "frame_buffer.h"
#include "gaussian.h"

namespace wcore
{

class Texture;
class BloomRenderer : public Renderer
{
private:
    Shader blur_pass_shader_;

    std::vector<std::shared_ptr<Texture>> bloom_h_tex_;
    std::vector<std::shared_ptr<Texture>> bloom_tex_;
    std::vector<FrameBuffer*> fbo_h_;
    FrameBuffer* fbo_;
    math::GaussianKernel kernel_;

public:
    BloomRenderer();
    virtual ~BloomRenderer();
    virtual void render(Scene* pscene) override;

    inline void update_blur_kernel(uint32_t kernel_size, float sigma)
    {
        kernel_.update_kernel(kernel_size, sigma);
    }
};

}

#endif // BLUR_PASS_H
