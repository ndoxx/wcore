#include "wcontext.h"

namespace wcore
{

AbstractContext::~AbstractContext()
{
    #ifndef __DISABLE_EDITOR__
        shutdown_imgui();
    #endif
}

void AbstractContext::center_cursor()
{
    // Get window size
    int win_width, win_height;
    get_window_size(win_width, win_height);
    set_cursor_position(win_width/2.0, win_height/2.0);
}

}
