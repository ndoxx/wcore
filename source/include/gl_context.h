#ifndef GL_CONTEXT_H
#define GL_CONTEXT_H

#include <functional>
#include <list>

#include "logger.h"

struct GLFWwindow;

namespace wcore
{

class GLContext
{
private:
    GLFWwindow* window_;
    bool cursor_hidden_;
    bool render_editor_GUI_;

    std::function<void(GLFWwindow*)> setup_func_;
    std::function<void(GLFWwindow*, float)> update_func_;
    std::function<void(void)> render_func_;

    std::list<std::function<void(void)>> editor_widget_generators_;

#ifndef __DISABLE_EDITOR__
    void generate_editor_widgets();
#endif

public:
    GLContext();
    ~GLContext();

    void _setup(std::function<void(GLFWwindow*)> before_loop)  { setup_func_ = before_loop; }
    void _update(std::function<void(GLFWwindow*, float)> update_func) { update_func_ = update_func;}
    void _render(std::function<void(void)> render_func) { render_func_ = render_func;}

#ifndef __DISABLE_EDITOR__
    inline void toggle_editor_GUI_rendering() { render_editor_GUI_ = !render_editor_GUI_; }
    inline void add_editor_widget_generator(std::function<void(void)> func) { editor_widget_generators_.push_back(func); }
    void imgui_new_frame();
#endif

    void toggle_cursor();

    int main_loop();
};

}

#endif // GL_CONTEXT_H
