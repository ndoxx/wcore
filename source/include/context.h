#ifndef CONTEXT_H
#define CONTEXT_H

struct GLFWwindow;

namespace wcore
{

class Context
{
public:
    friend class InputHandler;

    Context();
    ~Context();

    void swap_buffers();
    void poll_events();

    void toggle_hard_cursor();
    void hide_hard_cursor();
    void show_hard_cursor();
    void center_cursor();
    bool window_required();

#ifndef __DISABLE_EDITOR__
    void init_imgui();
#endif // __DISABLE_EDITOR__

private:
    GLFWwindow* window_;
    bool cursor_hidden_;
};

}

#endif // CONTEXT_H
