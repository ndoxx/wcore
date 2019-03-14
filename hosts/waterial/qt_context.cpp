#include "qt_context.h"

namespace medit
{

QtContext::~QtContext()
{

}

uint16_t QtContext::get_key_state(uint16_t key)
{
    return 0;
}

uint8_t QtContext::get_mouse_buttons_state()
{
    return 0;
}

void QtContext::get_window_size(int& width, int& height)
{
    width = 200;
    height = 200;
}

void QtContext::get_cursor_position(double& x, double& y)
{
    x = 0;
    y = 0;
}

void QtContext::set_cursor_position(double x, double y)
{

}

void QtContext::toggle_hard_cursor()
{

}

void QtContext::hide_hard_cursor()
{

}

void QtContext::show_hard_cursor()
{

}

bool QtContext::window_required()
{
    return true;
}

void QtContext::swap_buffers()
{

}

void QtContext::poll_events()
{

}


} // namespace medit
