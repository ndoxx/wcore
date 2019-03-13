#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <map>
#include <functional>

#include "xml_parser.h"
#include "informer.h"

namespace wcore
{

class AbstractContext;

struct KeyBindingProperties
{
public:
    KeyBindingProperties(uint16_t cooldown_reset_val,
                         uint16_t trigger,
                         uint16_t key,
                         bool     repeat):
    cooldown(0),
    cooldown_reset_val(cooldown_reset_val),
    trigger(trigger),
    key(key),
    repeat(repeat){}

    inline void hot()   { cooldown = cooldown_reset_val; }
    inline void cold()  { cooldown = 0; }
    inline void cool()  { if(cooldown) --cooldown; }
    inline bool ready() { return (!cooldown); }

    uint16_t cooldown;
    uint16_t cooldown_reset_val;
    uint16_t trigger;
    uint16_t key;
    bool     repeat;
};

class InputHandler : public Informer
{
private:
    XMLParser xml_parser_;
    std::map<hash_t, KeyBindingProperties>      key_bindings_; // Associate binding name to properties
    std::map<hash_t, std::function<void(void)>> action_map_;   // Associate key binding to action

    bool mouse_lock_;

public:
    InputHandler();
    ~InputHandler() = default;

    inline void lock_mouse()        { mouse_lock_ = true; }
    inline void unlock_mouse()      { mouse_lock_ = false; }
    inline void toggle_mouse_lock() { mouse_lock_ = !mouse_lock_; }
    inline bool is_mouse_locked() const { return mouse_lock_; }
    void toggle_cursor(AbstractContext& context);

    void import_key_bindings();

    void set_key_binding(hash_t name,
                         uint16_t key,
                         uint16_t cooldown = 0,
                         uint16_t trigger = 1, // W_KEY_PRESS
                         bool repeat = false);

    bool stroke_debounce(AbstractContext& context,
                         hash_t binding_name);

    void handle_keybindings(AbstractContext& context);

    void handle_mouse(AbstractContext& context);

private:
    inline void cooldown();
    inline void cooldown(hash_t binding_name);
    inline void hot(hash_t binding_name);
    inline void cold(hash_t binding_name);
    inline bool ready(hash_t binding_name);
};

inline void InputHandler::cooldown()
{
    for(auto pair: key_bindings_)
    {
        auto& kb = pair.second;
        if(kb.cooldown>0)
            --kb.cooldown;
    }
}

inline void InputHandler::cooldown(hash_t binding_name)
{
    key_bindings_.at(binding_name).cool();
}

inline void InputHandler::hot(hash_t binding_name)
{
    key_bindings_.at(binding_name).hot();
}

inline void InputHandler::cold(hash_t binding_name)
{
    key_bindings_.at(binding_name).cold();
}

inline bool InputHandler::ready(hash_t binding_name)
{
    return key_bindings_.at(binding_name).ready();
}

}

#endif // INPUT_HANDLER_H
