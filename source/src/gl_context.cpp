#include <stdexcept>
#include <thread>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "gl_context.h"
#include "arguments.h"
#include "config.h"
#include "globals.h"
#include "clock.hpp"

//GUI
#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "imgui/imgui_impl_glfw.h"
    #include "imgui/imgui_impl_opengl3.h"
#endif

#ifdef __PROFILING_GAMELOOP__
    #include "moving_average.h"
    #include "debug_info.h"
    #include "math3d.h"
#endif

namespace wcore
{

#ifdef __PROFILING_GAMELOOP__
    static MovingAverage render_time_fifo(1000);
    static MovingAverage update_time_fifo(1000);
    static MovingAverage idle_time_fifo(1000);
#endif

static void glfw_error_callback(int error, const char* description)
{
    std::stringstream ss;
    ss << "GLFW error code <v>" << error << "</v> :";
    DLOGE(ss.str(), "core", Severity::CRIT);
    DLOGI(description, "core", Severity::CRIT);
}

GLContext::GLContext():
window_(nullptr),
cursor_hidden_(true),
render_editor_GUI_(false)
{
    // Setup GLFW error callback
    glfwSetErrorCallback(glfw_error_callback);
    // Initialise GLFW
    if( !glfwInit() )
    {
        DLOGE("Failed to initialize GLFW.", "core", Severity::CRIT);
        throw std::runtime_error("Failed to initialize GLFW.");
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
        DLOGE("Failed to open GLFW window.", "core", Severity::CRIT);
        throw std::runtime_error("Failed to open GLFW window.");
    }

    glfwMakeContextCurrent(window_);
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window_, GLFW_STICKY_KEYS, GL_TRUE);
    // [BUG][glfw] glfwGetCursorPos does not update if cursor visible ???!!!
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // If not set, segfault at glGenVertexArrays()
    if (glewInit() != GLEW_OK) {
        DLOGE("Failed to initialize GLEW.", "core", Severity::CRIT);
        throw std::runtime_error("Failed to initialize GLEW.");
    }
    glGetError();   // Mask an unavoidable error caused by GLEW

    if(CONFIG.is(H_("root.display.vsync")))
        glfwSwapInterval(1); // Enable vsync

    // GUI initialization
#ifndef __DISABLE_EDITOR__
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 400 core");
    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    //ImGui::StyleColorsClassic();

    ImGuiStyle * style = &ImGui::GetStyle();

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
#endif

    // Clear window
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

GLContext::~GLContext()
{
#ifndef __DISABLE_EDITOR__
    // Shutdown ImGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif

    // Close OpenGL window and terminate GLFW
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void GLContext::toggle_cursor()
{
    cursor_hidden_ = !cursor_hidden_;
    if(cursor_hidden_)
    {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        // Get window size
        int win_width, win_height;
        glfwGetWindowSize(window_, &win_width, &win_height);
        // Reset mouse position for next frame
        glfwSetCursorPos(window_,
                         win_width/2,
                         win_height/2);
    }
    else
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

#ifndef __DISABLE_EDITOR__
void GLContext::imgui_new_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GLContext::generate_editor_widgets()
{
    for(auto&& func: editor_widget_generators_)
        func();
}
#endif

#ifdef __PROFILING_GAMELOOP__
#include <sstream>
#include <cmath>
#include <iomanip>
static std::string dbg_display_sub_duration(const std::string& name,
                                            float duration_us,
                                            float dt)
{
    float hot = fmin(1.0f, duration_us/dt);
    int R = int(fmin(1.0f, 2.0f*hot)*255.0f);
    int G = int(fmin(1.0f, 2.0f*(1.0f-hot))*255.0f);
    std::stringstream ss;
    ss << "\033[2K\033[1;38;2;" << std::to_string(R)
                         << ";" << std::to_string(G)
                         << ";0m" << name << ":\t"
 << std::setprecision(10) << 1e6*duration_us << "µs\t"
 << std::setprecision(5) << 100.0f*hot << "%\033[0m";
    return ss.str();
}
#endif

int GLContext::main_loop()
{
    uint32_t target_fps_ = 60;
    CONFIG.get(H_("root.display.target_fps"), target_fps_);

    const std::chrono::nanoseconds frame_duration_ns_(uint32_t(1e9*1.0f/target_fps_));

    nanoClock frame_clock;
    nanoClock clock;
    float dt = 0.0f;

#ifdef __PROFILING_GAMELOOP__
    nanoClock profile_clock;
    float dt_profile_render;
    float dt_profile_update;
    float dt_profile_events;
    float dt_profile_bufswp;
    // Register debug info fields
    DINFO.register_text_slot(H_("sdiFPS"), math::vec3(1.0,1.0,1.0));
    DINFO.register_text_slot(H_("sdiRender"), math::vec3(1.0,1.0,1.0));
#endif //__PROFILING_GAMELOOP__
#ifdef __PROFILING_STOP_AFTER_X_SAMPLES__
    uint32_t n_frames = 0;
#endif //__PROFILING_STOP_AFTER_X_SAMPLES__

    setup_func_(window_);
#ifdef __DEBUG__
    DLOGT("-------- Game loop start --------", "profile", Severity::LOW);
#endif
    do
    {
        // Restart timers
        frame_clock.restart();
        clock.restart();

        // GAME UPDATES
#ifdef __PROFILING_GAMELOOP__
        profile_clock.restart();
#endif //__PROFILING_GAMELOOP__
        update_func_(window_, dt);
#ifdef __PROFILING_GAMELOOP__
        {
            auto period = profile_clock.get_elapsed_time();
            dt_profile_update = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
            update_time_fifo.push(dt_profile_update);
        }
#endif //__PROFILING_GAMELOOP__

        // GUI
#ifndef __DISABLE_EDITOR__
        if(render_editor_GUI_)
            generate_editor_widgets();
#endif

        // RENDER
#ifdef __PROFILING_GAMELOOP__
        glFinish();
        profile_clock.restart();
#endif //__PROFILING_GAMELOOP__

        // Render game
        render_func_();
        // Render GUI
#ifndef __DISABLE_EDITOR__
        if(render_editor_GUI_)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
#endif

#ifdef __PROFILING_GAMELOOP__
        {
            glFinish();
            auto period = profile_clock.get_elapsed_time();
            dt_profile_render = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
            render_time_fifo.push(dt_profile_render);
        }
#endif //__PROFILING_GAMELOOP__

#ifdef __PROFILING_GAMELOOP__
        profile_clock.restart();
#endif //__PROFILING_GAMELOOP__
        // Swap buffers
        glfwSwapBuffers(window_);
#ifdef __PROFILING_GAMELOOP__
        {
            auto period = profile_clock.get_elapsed_time();
            dt_profile_bufswp = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        }
#endif //__PROFILING_GAMELOOP__

#ifdef __PROFILING_GAMELOOP__
        profile_clock.restart();
#endif //__PROFILING_GAMELOOP__
        glfwPollEvents();
#ifdef __PROFILING_GAMELOOP__
        {
            auto period = profile_clock.get_elapsed_time();
            dt_profile_events = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
        }
#endif //__PROFILING_GAMELOOP__

        auto frame_d = clock.restart();
#ifdef __PROFILING_GAMELOOP__
        float active_time = std::chrono::duration_cast<std::chrono::duration<float>>(frame_d).count();
#endif //__PROFILING_GAMELOOP__
        auto sleep_duration = frame_duration_ns_ - frame_d;
#ifdef __PROFILING_GAMELOOP__
        float sleep_time = std::chrono::duration_cast<std::chrono::duration<float>>(sleep_duration).count();
        idle_time_fifo.push(sleep_time);
#endif //__PROFILING_GAMELOOP__
        std::this_thread::sleep_for(sleep_duration);

        frame_d = frame_clock.restart();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(frame_d).count();

#ifdef __PROFILING_GAMELOOP__
        DINFO.display(H_("sdiFPS"), std::string("FPS: ") + std::to_string(1.0f/dt));
        DINFO.display(H_("sdiRender"), std::string("Render: ") + std::to_string(1e3*dt_profile_render)
                      + std::string("ms"));

#endif //__PROFILING_GAMELOOP__
#ifdef __PROFILING_STOP_AFTER_X_SAMPLES__
        if(++n_frames > 1200) break;
#endif //__PROFILING_STOP_AFTER_X_SAMPLES__
    }
    while(glfwGetKey(window_, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
          glfwWindowShouldClose(window_) == 0 );

#ifdef __DEBUG__
    DLOGT("-------- Game loop stop ---------", "profile", Severity::LOW);
#endif

#ifdef __PROFILING_GAMELOOP__
    FinalStatistics render_stats = render_time_fifo.get_stats();
    FinalStatistics update_stats = update_time_fifo.get_stats();
    FinalStatistics idle_stats   = idle_time_fifo.get_stats();
    uint32_t n_iter = render_time_fifo.get_size();

    DLOGN("Render time statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::DET);
    render_stats.debug_print(1e6, "µs", "profile");

    DLOGN("Update time statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::DET);
    update_stats.debug_print(1e6, "µs", "profile");

    DLOGN("Idle time statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile", Severity::DET);
    idle_stats.debug_print(1e6, "µs", "profile");
#endif //__PROFILING_GAMELOOP__

    dbg::LOG.write("debug.log");

    return 0;
}

}
