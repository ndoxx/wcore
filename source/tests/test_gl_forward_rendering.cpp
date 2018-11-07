#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstdlib>

#include "model.h"
#include "camera.h"
#include "shader.h"
#include "buffer_unit.hpp"
#include "gl_context.h"
#include "texture.h"
#include "scene.h"
#include "screen_renderer.h"
#include "vertex_array.hpp"
#include "lights.h"

using namespace math;

static const char* get_cmd_option(const char** begin, const char ** end, const std::string & option)
{
    const char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

/*static bool cmd_option_exists(const char** begin, const char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}*/

int main(int argc, char const *argv[])
{
    // Default window size
    unsigned int screenWidth  = 1024;
    unsigned int screenHeight = 768;

    // Screen size specified in arguments? Initialize screen size accordingly.
    const char* screenSize = get_cmd_option(argv, argv + argc, "-s");
    if(screenSize)
    {
        std::string screenSizeStr(screenSize);
        std::size_t xpos = screenSizeStr.find("x");
        screenWidth = std::stoi(screenSizeStr.substr(0, xpos));
        screenHeight = std::stoi(screenSizeStr.substr(xpos+1));
    }

    GLContext context(screenWidth, screenHeight);
    Shader shader("../res/shaders/lighting.vert",
                  "../res/shaders/lighting.geom",
                  "../res/shaders/lighting.frag");

    // In renderer class
    BufferUnit<Vertex3P3N2U> buffer_unit_;
    VertexArray<Vertex3P3N2U> vertex_array_(buffer_unit_);

    // Scene initialization
    Scene scene(std::make_shared<Camera>(screenWidth, screenHeight));
    std::shared_ptr<Camera> pcam = scene.get_camera();

    // Renders "screen" texture to a quad
    ScreenRenderer screen_renderer(screenWidth, screenHeight);
    auto pscreen = Texture::get_named_texture(H_("screen")).lock();

    // LOADING
    context._setup([&]()
    {
        // Submit models mesh to buffer unit then upload to OpenGL
        scene.traverse_models([&](std::shared_ptr<Model> pmodel)
        {
            std::cout << "  -> \033[1;38;2;255;200;0mSubmitting model: \033[0mnv=" << pmodel->get_mesh().get_nv()
                      << "\tni=" << pmodel->get_mesh().get_ni()
                      << "\tne=" << pmodel->get_mesh().get_n_elements() << std::endl;
            buffer_unit_.submit(pmodel->get_mesh());
        });
        buffer_unit_.upload();

        glEnable(GL_DEPTH_TEST);
    });

    float gamma = 0.0f;
    context._update([&](float dt)
    {
        // GAME UPDATES
        scene.update(dt);
        gamma += dt;
        if(gamma>10.0f) gamma = 10.0f;
    });

    context._render([&]()
    {
        // RENDERER: FIRST PASS
        // Get camera matrices
        mat4 V = pcam->get_view_matrix();       // Camera View matrix
        mat4 P = pcam->get_projection_matrix(); // Camera Projection matrix
        mat4 PV = P*V;

        // Activate shader and update uniforms
        shader.use();
        //shader.update_uniform_V(V);
        shader.update_uniform_eye(pcam->get_position());
        shader.update_uniform_wireframe_mix(pow(fabs(sin(0.1*M_PI*gamma)),3));

        // Send light uniforms
        scene.traverse_lights([&](std::shared_ptr<const Light> plight)
        {
            shader.update_uniform_light(plight);
        });

        // Render scene to named texture "screen"
        screen_renderer.with_frame_buffer_as_render_target([&]()
        {
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // Bind VAO, draw, unbind VAO
            vertex_array_.bind();
            scene.traverse_models([&](std::shared_ptr<Model> pmodel)
            {
                // Get model matrix and compute products
                mat4 M = pmodel->get_model_matrix();
                //mat4 MV = V*M;
                mat4 MVP = PV*M;

                shader.update_uniform_N(M.submatrix(3,3)); // Transposed inverse of M if non uniform scales
                shader.update_uniform_M(M);
                //shader.update_uniform_MV(MV);
                shader.update_uniform_MVP(MVP);
                shader.update_uniform_material(pmodel->get_material());

                pmodel->get_material().get_texture().bind_all();
                buffer_unit_.draw(pmodel->get_mesh().get_n_elements(),
                                  pmodel->get_mesh().get_buffer_offset());
                pmodel->get_material().get_texture().unbind();
            });
            vertex_array_.unbind();
        });
        shader.unuse();

        // RENDERER: SECOND PASS
        // Render "screen" texture to a quad
        screen_renderer.draw();
    });

    return context.main_loop();
}
