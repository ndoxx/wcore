#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <functional>
#include <list>

#include "logger.h"
#include "context.h"
#include "game_clock.h"
#include "input_handler.h"
#include "listener.h"

namespace wcore
{

class Updatable;

class GameLoop: public Listener
{
private:
    Context context_;
    GameClock game_clock_;
    InputHandler handler_;
    bool render_editor_GUI_;

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

    inline InputHandler& get_input_handler() { return handler_; }
    inline void set_render_func(std::function<void(void)> render_func) { render_func_ = render_func;}

    template <class T>
    inline void register_updatable_system(T& system)
    {
        updatables_.push_back(static_cast<Updatable*>(&system));
    }

#ifndef __DISABLE_EDITOR__
    inline void toggle_editor_GUI_rendering() { render_editor_GUI_ = !render_editor_GUI_; }
    inline void register_editor_widget(std::function<void(void)> func) { editor_widget_generators_.push_back(func); }
    void imgui_new_frame();
#endif

    inline void toggle_cursor() { context_.toggle_cursor(); }

    void onKeyboardEvent(const WData& data);
    void handle_events();
    int run();
};

}

#endif // ENGINE_CORE_H
