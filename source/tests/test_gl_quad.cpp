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
#include "bezier.h"
#include "material.h"
#include "texture.h"

using namespace math;


static const GLfloat g_quad_vertex_buffer_data[] = {
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
};

int main(int argc, char const *argv[])
{
    // Default window size
    unsigned int screenWidth  = 1024;
    unsigned int screenHeight = 768;

    GLContext context(screenWidth, screenHeight);

    GLuint VertexArrayID;
    GLuint quad_vertexbuffer;

    Shader quadShader("../res/shaders/quad.vert",
                      "../res/shaders/quad.frag");

    // Loading
    context._setup([&]()
    {
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);

        glGenBuffers(1, &quad_vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(g_quad_vertex_buffer_data),
                     g_quad_vertex_buffer_data,
                     GL_STATIC_DRAW);
    });

    // Game loop
    context._loop([&]()
    {
        // Render quad to screen
        // Render on the whole framebuffer, complete from the lower left corner to the upper right
        glViewport(0,0,screenWidth,screenHeight);

        // Clear the screen
        //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        quadShader.use();

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
        glVertexAttribPointer(
            0,                  // attribute 0
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            nullptr             // array buffer offset
        );

        // Draw the triangles !
        glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles

        glDisableVertexAttribArray(0);

        quadShader.unuse();

        // Frame control
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    });

    return context.main_loop();
}
