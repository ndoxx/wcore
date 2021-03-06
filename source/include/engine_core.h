#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <functional>
#include <list>

#include "logger.h"
#include "game_clock.h"
#include "input_handler.h"
#include "game_system.h"
#include "listener.h"

namespace wcore
{

class AbstractContext;
class EngineCore: public Listener
{
private:
    AbstractContext* context_;
    GameClock game_clock_;
    InputHandler handler_;
    bool render_editor_GUI_;

    std::function<void(void)> render_func_;
    std::function<void(void)> render_gui_func_;

    GameSystemContainer game_systems_;

#ifndef __DISABLE_EDITOR__
    void init_imgui();
    void generate_editor_widgets();
#endif

public:
    EngineCore(AbstractContext* context=nullptr);
    ~EngineCore();

    inline InputHandler& get_input_handler() { return handler_; }
    inline void set_render_func(std::function<void(void)> render_func) { render_func_ = render_func;}
    inline void set_render_gui_func(std::function<void(void)> render_func) { render_gui_func_ = render_func;}

    inline void register_initializer_system(hash_t name, InitializerSystem* system) { game_systems_.register_initializer_system(name, system); }
    inline void register_game_system(hash_t name, GameSystem* system)               { game_systems_.register_game_system(name, system, handler_); }
    inline void init_game_systems()           { game_systems_.init_game_systems(); }
    inline void init_system_parameters()      { game_systems_.init(); }
    inline void serialize_system_parameters() { game_systems_.serialize(); }
    inline void unload_game_systems()         { game_systems_.unload(); }

#ifndef __DISABLE_EDITOR__
    inline void toggle_editor_GUI_rendering() { render_editor_GUI_ = !render_editor_GUI_; }
    void imgui_new_frame();
#endif

    bool onKeyboardEvent(const WData& data);
    bool onMouseFocus(const WData& data);
    void handle_events();

    int run();

    // Functions to handle game loop from outside
    inline void render() { render_func_(); }
    void update(float dt);
    void swap_buffers();
    void poll_events();
    bool window_required();
};

}

#endif // ENGINE_CORE_H
