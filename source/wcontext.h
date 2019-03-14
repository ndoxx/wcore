#ifndef WCONTEXT_H
#define WCONTEXT_H

#include <memory>

namespace wcore
{

#define W_KEY_RELEASE 0
#define W_KEY_PRESS 1
#define W_KEY_REPEAT 2

class AbstractContext
{
public:
    virtual ~AbstractContext();
    virtual uint16_t get_key_state(uint16_t key) = 0;
    virtual uint8_t get_mouse_buttons_state() = 0;
    virtual void get_window_size(int& width, int& height) = 0;
    virtual void get_cursor_position(double& x, double& y) = 0;
    virtual void set_cursor_position(double x, double y) = 0;
    virtual void toggle_hard_cursor() = 0;
    virtual void hide_hard_cursor() = 0;
    virtual void show_hard_cursor() = 0;
    virtual bool window_required() = 0;
    virtual void swap_buffers() = 0;
    virtual void poll_events() = 0;

#ifndef __DISABLE_EDITOR__
    virtual void init_imgui() {}
    virtual void shutdown_imgui() {}
    virtual void imgui_new_frame() {}
    virtual void imgui_render() {}
    virtual bool imgui_initialized() { return false; }
#endif // __DISABLE_EDITOR__

    void center_cursor();
};

} // namespace wcore

#endif // WCONTEXT_H
