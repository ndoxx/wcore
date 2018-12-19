#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <thread>

//GUI
#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "imgui/imgui_impl_glfw.h"
    #include "imgui/imgui_impl_opengl3.h"
#endif

#include "context.h"
#include "config.h"
#include "logger.h"
#include "error.h"
#include "globals.h"

namespace wcore
{

static void glfw_error_callback(int error, const char* description)
{
    std::stringstream ss;
    ss << "GLFW error code <v>" << error << "</v> :";
    DLOGE(ss.str(), "core", Severity::CRIT);
    DLOGI(description, "core", Severity::CRIT);
}

Context::Context():
window_(nullptr),
cursor_hidden_(true)
{
    // Setup GLFW error callback
    glfwSetErrorCallback(glfw_error_callback);

    // Initialise GLFW
    if( !glfwInit() )
    {
        DLOGF("Failed to initialize GLFW.", "core", Severity::CRIT);
        fatal("Failed to initialize GLFW.");
    }

    glGetError(); // Hide glfwInit's errors
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    if(GLB.SCR_FULL)
        window_ = glfwCreateWindow(GLB.SCR_W, GLB.SCR_H, "WCore", glfwGetPrimaryMonitor(), NULL);
    else
        window_ = glfwCreateWindow(GLB.SCR_W, GLB.SCR_H, "WCore", NULL, NULL);

    if( window_ == NULL )
    {
        DLOGF("Failed to open GLFW window.", "core", Severity::CRIT);
        fatal("Failed to open GLFW window.");
    }

    glfwMakeContextCurrent(window_);
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window_, GLFW_STICKY_KEYS, GL_TRUE);
    // [BUG][glfw] glfwGetCursorPos does not update if cursor visible ???!!!
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // If not set, segfault at glGenVertexArrays()
    if (glewInit() != GLEW_OK) {
        DLOGF("Failed to initialize GLEW.", "core", Severity::CRIT);
        fatal("Failed to initialize GLEW.");
    }
    glGetError();   // Mask an unavoidable error caused by GLEW

    if(CONFIG.is(H_("root.display.vsync")))
        glfwSwapInterval(1); // Enable vsync

    // Sleep a bit so that the window has enough time to open, and the sizes we get
    // actually correspond to the window size
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    glfwGetWindowSize(window_, &GLB.WIN_W, &GLB.WIN_H);
}

Context::~Context()
{
    // Close OpenGL window and terminate GLFW
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void Context::swap_buffers()
{
/*#ifdef __PROFILING_GAMELOOP__
        profile_clock.restart();
#endif //__PROFILING_GAMELOOP__*/

    glfwSwapBuffers(window_);

/*#ifdef __PROFILING_GAMELOOP__
        {
            auto period = profile_clock.get_elapsed_time();
            dt_profile_bufswp = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        }
#endif //__PROFILING_GAMELOOP__*/
}

void Context::poll_events()
{
    glfwPollEvents();
}

void Context::toggle_hard_cursor()
{
    cursor_hidden_ = !cursor_hidden_;
    if(cursor_hidden_)
    {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Context::show_hard_cursor()
{
    cursor_hidden_ = false;
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Context::hide_hard_cursor()
{
    cursor_hidden_ = true;
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Context::center_cursor()
{
    //if(cursor_hidden_)
    //{
        // Get window size
        int win_width, win_height;
        glfwGetWindowSize(window_, &win_width, &win_height);
        // Reset mouse position for next frame
        glfwSetCursorPos(window_,
                         win_width/2,
                         win_height/2);
    //}
}

bool Context::window_required()
{
    return ((glfwGetKey(window_, GLFW_KEY_ESCAPE ) != GLFW_PRESS)
           && (glfwWindowShouldClose(window_) == 0));
}

#ifndef __DISABLE_EDITOR__
void Context::init_imgui()
{
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
}
#endif // __DISABLE_EDITOR__

}
