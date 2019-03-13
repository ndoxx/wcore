#ifndef CONTEXT_H
#define CONTEXT_H

#include "wcontext.h"

struct GLFWwindow;

namespace wcore
{

class GLFWContext: public AbstractContext
{
public:
    friend class InputHandler;

    GLFWContext();
    ~GLFWContext();

    virtual uint16_t get_key_state(uint16_t key) override;
    virtual uint8_t get_mouse_buttons_state() override;
    virtual void get_window_size(int& width, int& height) override;
    virtual void get_cursor_position(double& x, double& y) override;
    virtual void set_cursor_position(double x, double y) override;
    virtual void toggle_hard_cursor() override;
    virtual void hide_hard_cursor() override;
    virtual void show_hard_cursor() override;
    virtual bool window_required() override;
    virtual void swap_buffers() override;
    virtual void poll_events() override;

#ifndef __DISABLE_EDITOR__
    virtual void init_imgui() override;
#endif // __DISABLE_EDITOR__

protected:
    class GLFWImpl;
    std::shared_ptr<GLFWImpl> pimpl_; // opaque pointer to context implementation

private:
    bool cursor_hidden_;
};

}

#endif // CONTEXT_H
