#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <functional>
#include <list>

#include "logger.h"

struct GLFWwindow;

namespace wcore
{

class GameLoop
{
private:
    GLFWwindow* window_;
    bool cursor_hidden_;
    bool render_editor_GUI_;

    std::function<void(GLFWwindow*, float)> update_func_;
    std::function<void(void)> render_func_;

    std::list<std::function<void(void)>> editor_widget_generators_;

#ifndef __DISABLE_EDITOR__
    void generate_editor_widgets();
#endif

public:
    GameLoop();
    ~GameLoop();

    void _update(std::function<void(GLFWwindow*, float)> update_func) { update_func_ = update_func;}
    void _render(std::function<void(void)> render_func) { render_func_ = render_func;}

#ifndef __DISABLE_EDITOR__
    inline void toggle_editor_GUI_rendering() { render_editor_GUI_ = !render_editor_GUI_; }
    inline void add_editor_widget_generator(std::function<void(void)> func) { editor_widget_generators_.push_back(func); }
    void imgui_new_frame();
#endif

    void swap_buffers();
    void poll_events();
    void toggle_cursor();

    int main_loop();
};

}

#endif // ENGINE_CORE_H
