#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <functional>
#include <list>

#include "logger.h"
#include "context.h"
#include "game_clock.h"

namespace wcore
{

class Updatable;
class InputHandler;

class GameLoop
{
private:
    Context context_;
    GameClock game_clock_;
    bool render_editor_GUI_;

    std::function<void(Context&, float)> update_func_;
    std::function<void(void)> render_func_;

    std::list<std::function<void(void)>> editor_widget_generators_;
    std::list<Updatable*> updatables_;

#ifndef __DISABLE_EDITOR__
    void init_imgui();
    void generate_editor_widgets();
#endif

public:
    GameLoop();
    ~GameLoop();

    void _update(std::function<void(Context&, float)> update_func) { update_func_ = update_func;}
    void set_render_func(std::function<void(void)> render_func) { render_func_ = render_func;}

    inline void register_updatable_system(Updatable* system) { updatables_.push_back(system); }

#ifndef __DISABLE_EDITOR__
    inline void toggle_editor_GUI_rendering() { render_editor_GUI_ = !render_editor_GUI_; }
    inline void register_editor_widget(std::function<void(void)> func) { editor_widget_generators_.push_back(func); }
    void imgui_new_frame();
#endif

    inline void toggle_cursor() { context_.toggle_cursor(); }

    void setup_user_inputs(InputHandler& handler);

    int main_loop();
};

}

#endif // ENGINE_CORE_H
