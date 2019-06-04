#include <thread>
#include <sstream>

#include "engine_core.h"
#include "glfwcontext.h"
#include "config.h"
#include "globals.h"
#include "game_system.h"
#include "clock.hpp"
#include "error.h"
#include "input_handler.h"
#include "logger.h"

//GUI
#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
#endif

#ifdef __PROFILING_EngineCore__
    #include "moving_average.h"
    #include "debug_info.h"
    #include "math3d.h"
#endif

namespace wcore
{

#ifdef __PROFILING_EngineCore__
    static MovingAverage render_time_fifo(1000);
    static MovingAverage update_time_fifo(1000);
    static MovingAverage idle_time_fifo(1000);
#endif

EngineCore::EngineCore(AbstractContext* context):
context_(context),
render_editor_GUI_(false)
{
    // If no context was specified, create a GLFW context
    if(context_ == nullptr)
        context_ = new GLFWContext();

    // GUI initialization
#ifndef __DISABLE_EDITOR__
    context_->init_imgui();
    subscribe("input.keyboard"_h, handler_, &EngineCore::onKeyboardEvent);
#endif
    subscribe("input.mouse.focus"_h, handler_, &EngineCore::onMouseFocus);
}

EngineCore::~EngineCore()
{
    // Will shutdown imgui if used
    delete context_;
}

#ifndef __DISABLE_EDITOR__
static bool show_log_window = false;
void EngineCore::generate_editor_widgets()
{
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(365,GLB.WIN_H), ImGuiCond_Once);
    ImGui::Begin("Debug Menu");

    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if(ImGui::CollapsingHeader("Main debug options"))
        if(ImGui::Button("Show log window"))
            show_log_window = !show_log_window;

    game_systems_.generate_widgets();

    if(show_log_window)
        dbg::LOG.generate_widget();

    ImGui::End();
}
#endif

#ifdef __PROFILING_EngineCore__
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

bool EngineCore::onKeyboardEvent(const WData& data)
{
    const KbdData& kbd = static_cast<const KbdData&>(data);

    switch(kbd.key_binding)
    {
#ifndef __DISABLE_EDITOR__
        case "k_tg_editor"_h:
            handler_.toggle_mouse_lock();
            toggle_editor_GUI_rendering();
            context_->center_cursor();
            if(!CONFIG.is("root.gui.cursor.custom"_h))
                context_->toggle_hard_cursor();
        break;
#endif
        case "k_tg_pause"_h:
    		game_clock_.toggle_pause();
    		break;
        case "k_frame_speed_up"_h:
    		game_clock_.frame_speed_up();
    		break;
        case "k_frame_slow_down"_h:
    		game_clock_.frame_slow_down();
    		break;
        case "k_next_frame"_h:
    		game_clock_.require_next_frame();
    		break;
    }

    return true; // Do NOT consume event
}

bool EngineCore::onMouseFocus(const WData& data)
{
    if(CONFIG.is("root.gui.cursor.custom"_h))
    {
        const MouseFocusData& mfd = static_cast<const MouseFocusData&>(data);
        if(mfd.leaving_window)
            context_->show_hard_cursor();
        else
            context_->hide_hard_cursor();
    }

    return true; // Do NOT consume event
}

void EngineCore::handle_events()
{
#ifndef __DISABLE_EDITOR__
    if(context_->imgui_initialized())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureMouse)    // Don't propagate mouse events to game if ImGui wants them
            handler_.handle_mouse(*context_);
        if(!io.WantCaptureKeyboard) // Don't propagate keyboard events to game if ImGui wants them
            handler_.handle_keybindings(*context_);
    }
#else
    handler_.handle_mouse(*context_);
    handler_.handle_keybindings(*context_);
#endif
}

void EngineCore::update(float dt)
{
    if(game_clock_.is_game_paused())
        return;
    game_clock_.update(dt);

    for(auto&& system: game_systems_)
        system->update(game_clock_);

    // To allow frame by frame update
    game_clock_.release_flags();
}

void EngineCore::swap_buffers()
{
    context_->swap_buffers();
}

void EngineCore::poll_events()
{
    context_->poll_events();
}

bool EngineCore::window_required()
{
    return context_->window_required();
}

int EngineCore::run()
{
    uint32_t target_fps_ = 60;
    CONFIG.get("root.display.target_fps"_h, target_fps_);

    const std::chrono::nanoseconds frame_duration_ns_(uint32_t(1e9*1.0f/target_fps_));

    nanoClock frame_clock;
    nanoClock clock;
    float dt = 1.0f/target_fps_; // Set to non-zero value to avoid 1st frame render bug

#ifdef __PROFILING_EngineCore__
    nanoClock profile_clock;
    float dt_profile_render;
    float dt_profile_update;
    float dt_profile_events;
    float dt_profile_bufswp;
    // Register debug info fields
    DINFO.register_text_slot("sdiFPS"_h, math::vec3(1.0,1.0,1.0));
    DINFO.register_text_slot("sdiRender"_h, math::vec3(1.0,1.0,1.0));
#endif //__PROFILING_EngineCore__
#ifdef __PROFILING_STOP_AFTER_X_SAMPLES__
    uint32_t n_frames = 0;
#endif //__PROFILING_STOP_AFTER_X_SAMPLES__

#ifdef __DEBUG__
    DLOGT("-------- Game loop start --------", "profile");
#endif
    do
    {
        // Restart timers
        frame_clock.restart();
        clock.restart();

        // GAME UPDATES
#ifdef __PROFILING_EngineCore__
        profile_clock.restart();
#endif //__PROFILING_EngineCore__
        handle_events();
        // Start the Dear ImGui frame
#ifndef __DISABLE_EDITOR__
        context_->imgui_new_frame();
#endif
        // GAME UPDATES
        update(dt);

#ifdef __PROFILING_EngineCore__
        {
            auto period = profile_clock.get_elapsed_time();
            dt_profile_update = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
            update_time_fifo.push(dt_profile_update);
        }
#endif //__PROFILING_EngineCore__

        // RENDER
#ifdef __PROFILING_EngineCore__
        glFinish();
        profile_clock.restart();
#endif //__PROFILING_EngineCore__

        // Render game
        if(!game_clock_.is_game_paused())
            render();

        // GUI
#ifndef __DISABLE_EDITOR__
        if(render_editor_GUI_)
            generate_editor_widgets();
#endif

        // Render GUI
#ifndef __DISABLE_EDITOR__
        if(render_editor_GUI_)
            context_->imgui_render();
#endif
        render_gui_func_(); // Game GUI

#ifdef __PROFILING_EngineCore__
        {
            glFinish();
            auto period = profile_clock.get_elapsed_time();
            dt_profile_render = std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
            render_time_fifo.push(dt_profile_render);
        }
#endif //__PROFILING_EngineCore__

        // Finish game loop
        context_->swap_buffers();
        context_->poll_events();

        // Sleep for the rest of the frame
        auto frame_d = clock.restart();
        auto sleep_duration = frame_duration_ns_ - frame_d;

#ifdef __PROFILING_EngineCore__
        float sleep_time = std::chrono::duration_cast<std::chrono::duration<float>>(sleep_duration).count();
        idle_time_fifo.push(sleep_time);
#endif //__PROFILING_EngineCore__

        std::this_thread::sleep_for(sleep_duration);

        frame_d = frame_clock.restart();
        dt = std::chrono::duration_cast<std::chrono::duration<float>>(frame_d).count();

#ifdef __PROFILING_EngineCore__
        DINFO.display("sdiFPS"_h, std::string("FPS: ") + std::to_string(1.0f/dt));
        DINFO.display("sdiRender"_h, std::string("Render: ") + std::to_string(1e3*dt_profile_render) + std::string("ms"));
#endif //__PROFILING_EngineCore__

#ifdef __PROFILING_STOP_AFTER_X_SAMPLES__
        if(++n_frames > 1200) break;
#endif //__PROFILING_STOP_AFTER_X_SAMPLES__
    }
    while(context_->window_required());

#ifdef __DEBUG__
    DLOGT("-------- Game loop stop ---------", "profile");
#endif

#ifdef __PROFILING_EngineCore__
    FinalStatistics render_stats = render_time_fifo.get_stats();
    FinalStatistics update_stats = update_time_fifo.get_stats();
    FinalStatistics idle_stats   = idle_time_fifo.get_stats();
    uint32_t n_iter = render_time_fifo.get_size();

    DLOGN("Render time statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
    render_stats.debug_print(1e6, "µs", "profile");

    DLOGN("Update time statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
    update_stats.debug_print(1e6, "µs", "profile");

    DLOGN("Idle time statistics (over <z>" + std::to_string(n_iter) + "</z> points): ", "profile");
    idle_stats.debug_print(1e6, "µs", "profile");
#endif //__PROFILING_EngineCore__

    fs::path log_path;
    if(CONFIG.get<fs::path>("root.folders.log"_h, log_path))
        dbg::LOG.write(log_path / "debug.log");
    else
        dbg::LOG.write(fs::path("debug.log"));

    return 0;
}

}
