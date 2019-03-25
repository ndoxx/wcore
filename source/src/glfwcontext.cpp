#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <thread>

//GUI
#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "imgui/examples/imgui_impl_glfw.h"
    #include "imgui/examples/imgui_impl_opengl3.h"
#endif

#include "glfwcontext.h"
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

class GLFWContext::GLFWImpl
{
public:
    GLFWImpl();
    ~GLFWImpl();

    GLFWwindow* window_;
};

GLFWContext::GLFWImpl::GLFWImpl():
window_(nullptr)
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

    if(CONFIG.is("root.display.vsync"_h))
        glfwSwapInterval(1); // Enable vsync

    // UNSAFE Sleep a bit so that the window has enough time to open, and the sizes we get
    // actually correspond to the window size
    // NOTE(ndx) -> Find a way to do this with a callback
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //glfwGetWindowSize(window_, &GLB.WIN_W, &GLB.WIN_H);
    glfwGetFramebufferSize(window_, &GLB.WIN_W, &GLB.WIN_H);
}

GLFWContext::GLFWImpl::~GLFWImpl()
{
    // Close OpenGL window and terminate GLFW
    glfwDestroyWindow(window_);
    glfwTerminate();
    // call XkbFreeKeyboard(desc, 0, True) to suppress memory leak caused
    // by XkbGetMap in glfwInit for GLFW3.1. Leak corrected in 3.2.
}

GLFWContext::GLFWContext():
pimpl_(new GLFWImpl()),
cursor_hidden_(true)
{

}

GLFWContext::~GLFWContext()
{

}

uint16_t GLFWContext::get_key_state(uint16_t key)
{
    return (uint16_t)glfwGetKey(pimpl_->window_, key);
}

uint8_t GLFWContext::get_mouse_buttons_state()
{
    return (glfwGetMouseButton(pimpl_->window_, GLFW_MOUSE_BUTTON_LEFT)   << MouseButton::LMB)
         + (glfwGetMouseButton(pimpl_->window_, GLFW_MOUSE_BUTTON_RIGHT)  << MouseButton::RMB)
         + (glfwGetMouseButton(pimpl_->window_, GLFW_MOUSE_BUTTON_MIDDLE) << MouseButton::MMB);

}

void GLFWContext::get_window_size(int& width, int& height)
{
    glfwGetWindowSize(pimpl_->window_, &width, &height);
}

void GLFWContext::get_cursor_position(double& x, double& y)
{
    glfwGetCursorPos(pimpl_->window_, &x, &y);
}

void GLFWContext::set_cursor_position(double x, double y)
{
    glfwSetCursorPos(pimpl_->window_, x, y);
}

void GLFWContext::swap_buffers()
{
/*#ifdef __PROFILING_GAMELOOP__
        profile_clock.restart();
#endif //__PROFILING_GAMELOOP__*/

    glfwSwapBuffers(pimpl_->window_);

/*#ifdef __PROFILING_GAMELOOP__
        {
            auto period = profile_clock.get_elapsed_time();
            dt_profile_bufswp = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        }
#endif //__PROFILING_GAMELOOP__*/
}

void GLFWContext::poll_events()
{
    glfwPollEvents();
}

void GLFWContext::toggle_hard_cursor()
{
    cursor_hidden_ = !cursor_hidden_;
    if(cursor_hidden_)
    {
        glfwSetInputMode(pimpl_->window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
        glfwSetInputMode(pimpl_->window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void GLFWContext::show_hard_cursor()
{
    cursor_hidden_ = false;
    glfwSetInputMode(pimpl_->window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void GLFWContext::hide_hard_cursor()
{
    cursor_hidden_ = true;
    glfwSetInputMode(pimpl_->window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

bool GLFWContext::window_required()
{
    return ((glfwGetKey(pimpl_->window_, GLFW_KEY_ESCAPE ) != GLFW_PRESS)
           && (glfwWindowShouldClose(pimpl_->window_) == 0));
}

#ifndef __DISABLE_EDITOR__
static bool imgui_initialized_ = false;
void GLFWContext::init_imgui()
{
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(pimpl_->window_, true);
    ImGui_ImplOpenGL3_Init("#version 400 core");
    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();

    ImGuiStyle* style = &ImGui::GetStyle();

    ImVec4 neutral(0.7f, 0.3f, 0.05f, 1.0f);
    ImVec4 active(1.0f, 0.5f, 0.05f, 1.00f);
    ImVec4 hovered(0.6f, 0.6f, 0.6f, 1.00f);
    ImVec4 inactive(0.7f, 0.3f, 0.05f, 0.75f);

    style->WindowRounding = 5.0f;
    //style->Colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.75f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.7f, 0.3f, 0.05f, 0.75f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    style->Colors[ImGuiCol_TitleBg] = neutral;
    style->Colors[ImGuiCol_TitleBgCollapsed] = inactive;
    style->Colors[ImGuiCol_TitleBgActive] = active;

    style->Colors[ImGuiCol_Header] = neutral;
    style->Colors[ImGuiCol_HeaderHovered] = hovered;
    style->Colors[ImGuiCol_HeaderActive] = active;

    style->Colors[ImGuiCol_ResizeGrip] = neutral;
    style->Colors[ImGuiCol_ResizeGripHovered] = hovered;
    style->Colors[ImGuiCol_ResizeGripActive] = active;

    style->Colors[ImGuiCol_Button] = neutral;
    style->Colors[ImGuiCol_ButtonHovered] = hovered;
    style->Colors[ImGuiCol_ButtonActive] = active;

    style->Colors[ImGuiCol_SliderGrab] = hovered;
    style->Colors[ImGuiCol_SliderGrabActive] = active;

    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.1f, 0.8f, 0.2f, 1.0f);

    imgui_initialized_ = true;
}
void GLFWContext::shutdown_imgui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
void GLFWContext::imgui_new_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void GLFWContext::imgui_render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
bool GLFWContext::imgui_initialized()
{
    return imgui_initialized_;
}
#endif // __DISABLE_EDITOR__

}
