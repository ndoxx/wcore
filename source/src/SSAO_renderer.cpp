#include <random>
#include <vector>
#include <cassert>

#include "SSAO_renderer.h"
#include "gfx_driver.h"
#include "SSAO_buffer.h"
#include "g_buffer.h"
#include "l_buffer.h"
#include "scene.h"
#include "logger.h"
#include "camera.h"
#include "lights.h"
#include "texture.h"

namespace wcore
{

using namespace math;

uint32_t SSAORenderer::NOISE_SQRSIZE_ = 16;
uint32_t SSAORenderer::KERNEL_SQRSIZE_ = 8;
uint32_t SSAORenderer::KERNEL_SIZE_ = pow(SSAORenderer::KERNEL_SQRSIZE_,2);
uint32_t SSAORenderer::NOISE_SIZE_ = pow(SSAORenderer::NOISE_SQRSIZE_,2);


SSAORenderer::SSAORenderer():
Renderer<Vertex3P>(),
SSAO_shader_(ShaderResource("SSAO.vert;SSAO.frag")),
ping_pong_(ShaderResource("blurpass.vert;blurpass.frag", "VARIANT_COMPRESS_R"),
           std::make_unique<Texture>(
                       std::vector<hash_t>{H_("SSAOtmpTex")},
                       SSAOBuffer::Instance().get_width()/2,
                       SSAOBuffer::Instance().get_height()/2,
                       GL_TEXTURE_2D,
                       GL_LINEAR,
                       GL_RED,
                       GL_RGB,
                       true)),
out_size_(SSAOBuffer::Instance().get_width(),
          SSAOBuffer::Instance().get_height()),
noise_scale_(out_size_/(float(NOISE_SQRSIZE_))),
active_(true),
SSAO_radius_(0.20),
SSAO_bias_(0.025),
SSAO_vbias_(0.086),
SSAO_intensity_(1.3),
SSAO_scale_(0.375),
blur_policy_(1,
             SSAOBuffer::Instance().get_width(),
             SSAOBuffer::Instance().get_height(),
             0.67f,
             1.0f)
{
    load_geometry();
    generate_random_kernel();
    SSAOBuffer::Instance();
}

SSAORenderer::~SSAORenderer()
{
    SSAOBuffer::Kill();
}

void SSAORenderer::render()
{
    if(!active_) return;

    // For position reconstruction
    const math::mat4& P = SCENE.get_camera()->get_projection_matrix(); // Camera Projection matrix
    math::vec4 proj_params(1.0f/P(0,0), 1.0f/P(1,1), P(2,2)-1.0f, P(2,3));
    auto pgbuffer = Texture::get_named_texture(H_("gbuffer")).lock();

    GBuffer& gbuffer = GBuffer::Instance();
    //LBuffer& lbuffer = LBuffer::Instance();
    SSAOBuffer& ssaobuffer = SSAOBuffer::Instance();

    SSAO_shader_.use();
    SSAO_shader_.send_uniform(H_("rd.v2_texelSize"), vec2(1.0f/out_size_.x(),1.0f/out_size_.y()));
    // Render textured quad to screen
    glViewport(0,0,out_size_.x(),out_size_.y());

    // Bind textures
    gbuffer.bind_as_source(0,0);  // normal
    GFX::bind_texture2D(1, noise_texture_); // random rotations
    gbuffer.bind_as_source(2,2);  // depth
    //GFX::bind_texture2D(3, kernel_texture_); // random field (kernel)
    //gbuffer.bind_as_source(3,1);  // albedo
    //lbuffer.bind_as_source(3,0); // last frame color

    SSAO_shader_.send_uniform<int>(H_("normalTex"), 0);
    SSAO_shader_.send_uniform<int>(H_("noiseTex"), 1);
    SSAO_shader_.send_uniform<int>(H_("depthTex"), 2);
    //SSAO_shader_.send_uniform<int>(H_("albedoTex"), 3);

    // Render SSAO texture
    ssaobuffer.bind_as_target();
    GFX::clear_color();
    // Send samples to shader
    /*for(uint32_t ii=0; ii<KERNEL_SIZE_; ++ii)
    {
        SSAO_shader_.send_uniform_vec3(("v3_samples[" + std::to_string(ii) + "]").c_str(),
                                       ssao_kernel_[ii]);
    }*/

    // Send uniforms
    const math::mat4& V = SCENE.get_camera()->get_view_matrix();
    SSAO_shader_.send_uniform(H_("rd.v2_noiseScale"), noise_scale_);
    //if(auto dir_light = SCENE.get_directional_light().lock())
        //SSAO_shader_.send_uniform(H_("rd.v3_lightDir"), V.submatrix(3,3)*dir_light->get_position());
    SSAO_shader_.send_uniform(H_("rd.f_radius"), SSAO_radius_);
    SSAO_shader_.send_uniform(H_("rd.f_bias"), SSAO_bias_);
    SSAO_shader_.send_uniform(H_("rd.f_vbias"), SSAO_vbias_);
    SSAO_shader_.send_uniform(H_("rd.f_intensity"), SSAO_intensity_);
    SSAO_shader_.send_uniform(H_("rd.f_scale"), SSAO_scale_);
    //SSAO_shader_.send_uniform(H_("rd.b_invert_normals"), false);
    // For position reconstruction
    SSAO_shader_.send_uniform(H_("rd.v4_proj_params"), proj_params);

    vertex_array_.bind();
    buffer_unit_.draw(2, 0);
    ssaobuffer.unbind_as_target();
    gbuffer.unbind_as_source();

    SSAO_shader_.unuse();

    // Blur pass on occlusion texture
    if(blur_policy_.n_pass_)
    {
        //GFX::disable_face_culling();
        vertex_array_.bind();
        ping_pong_.run(*static_cast<BufferModule*>(&ssaobuffer),
                       blur_policy_,
                       [&]()
                       {
                            GFX::clear_color();
                            buffer_unit_.draw(2, 0);
                       });
    }
}

void SSAORenderer::generate_random_kernel()
{
    std::uniform_real_distribution<float> rnd_f(0.0f, 1.0f);
    std::default_random_engine rng(42);

    // Generate a random hemispherical distribution of vectors in tangent space
    for (uint32_t ii=0; ii<KERNEL_SIZE_; ++ii)
    {
        // Random vector in tangent space, z is always positive (hemispherical constraint)
        vec3 sample(rnd_f(rng) * 2.0f - 1.0f,
                    rnd_f(rng) * 2.0f - 1.0f,
                    rnd_f(rng));
        sample.normalize();
        sample *= rnd_f(rng);

        // Accelerating lerp on scale to increase weight of occlusion near to the
        // fragment we're testing for SSAO
        float scale = ii/float(KERNEL_SIZE_);
        scale   = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;

        ssao_kernel_.push_back(sample);
    }

    // Generate a random rotation vector distribution to act on kernel
    std::vector<vec3> ssao_noise;
    for (uint32_t ii=0; ii<NOISE_SIZE_; ++ii)
    {
        vec3 noise(rnd_f(rng) * 2.0f - 1.0f,
                   rnd_f(rng) * 2.0f - 1.0f,
                   0.0f);
        noise.normalize();
        ssao_noise.push_back(noise);
    }

    // TMP
    // Push kernel to a 1x64 1D GL texture
    /*glGenTextures(1, &kernel_texture_);
    glBindTexture(GL_TEXTURE_1D, kernel_texture_);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, KERNEL_SIZE_, 0, GL_RGB, GL_FLOAT, &ssao_kernel_[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);*/

    // Push noise to a GL texture that tiles the screen (GL_REPEAT)
    glGenTextures(1, &noise_texture_);
    glBindTexture(GL_TEXTURE_2D, noise_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, NOISE_SQRSIZE_, NOISE_SQRSIZE_, 0, GL_RGB, GL_FLOAT, &ssao_noise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
}

}
