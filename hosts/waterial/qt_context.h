#ifndef QT_CONTEXT_H
#define QT_CONTEXT_H

#include "wcontext.h"

namespace waterial
{

class QtContext: public wcore::AbstractContext
{
public:
    virtual ~QtContext();
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

    void center_cursor();
};


}

#endif // QT_CONTEXT_H
