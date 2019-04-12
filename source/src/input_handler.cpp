#include <GL/glew.h>

#include "xml_utils.hpp"
#include "input_handler.h"
#include "logger.h"
#include "keymap.h"
#include "wcontext.h"
#include "globals.h"
#include "file_system.h"
#include "error.h"

namespace wcore
{

static const char* keybindingsfile = "keybindings.xml";

using namespace rapidxml;

InputHandler::InputHandler():
mouse_lock_(true)
{
    import_key_bindings();
}

void InputHandler::import_key_bindings()
{
    DLOGS("[InputHandler] Parsing key bindings.", "input", Severity::LOW);

    auto pstream = FILESYSTEM.get_file_as_stream(keybindingsfile, "root.folders.config"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGE("[InputHandler] Unable to open file:", "input");
        DLOGI(keybindingsfile, "input");
        fatal();
    }
    xml_parser_.load_file_xml(*pstream);

    for (xml_node<>* cat=xml_parser_.get_root()->first_node("Category");
         cat; cat=cat->next_sibling("Category"))
    {
        std::string category;
        xml::parse_attribute(cat, "name", category);
        DLOGI("Parsing category: <x>" + category + "</x>", "input");

        for (xml_node<>* kb=cat->first_node("KB");
             kb; kb=kb->next_sibling("KB"))
        {
            uint16_t cooldown = 0,
                     trigger  = W_KEY_PRESS,
                     key      = 0;
            bool repeat = false;
            std::string str_name, str_key, str_trigger;

            // Mandatory attributes
            if(!xml::parse_attribute(kb, "name", str_name) ||
               !xml::parse_attribute(kb, "key", str_key))
                continue;

            // Lookup key value
            auto it = keymap::NAMES.find(H_(str_key.c_str()));
            if(it == keymap::NAMES.end())
            {
                DLOGW("[InputHandler] Unknown key name:", "input");
                DLOGI(str_key, "input");
                continue;
            }
            key = it->second;

            // Optional attributes
            xml::parse_attribute(kb, "cooldown", cooldown);
            xml::parse_attribute(kb, "repeat", repeat);
            if(xml::parse_attribute(kb, "trigger", str_trigger))
            {
                if(!str_trigger.compare("release"))
                    trigger = W_KEY_RELEASE;
                else
                    trigger = W_KEY_PRESS;
            }

            std::string str_desc;
            if(xml::parse_attribute(kb, "description", str_desc))
            {
#ifdef __DEBUG__
                std::stringstream ss;
                ss << "[<i>" << ((trigger==W_KEY_PRESS)?"PRESS":"RELEASE")
                   << "</i>] <n>" << str_key << "</n> -> " << str_desc;
                DLOG(ss.str(), "input", Severity::DET);
#endif
            }

            // Register key binding
            set_key_binding(H_(str_name.c_str()),
                            key,
                            cooldown,
                            trigger,
                            repeat);
        }
    }

    DLOGES("input", Severity::LOW);
}

void InputHandler::set_key_binding(hash_t name,
                                   uint16_t key,
                                   uint16_t cooldown,
                                   uint16_t trigger,
                                   bool repeat)
{
    auto it = key_bindings_.find(name);
    if(it != key_bindings_.end())
    {
#ifdef __DEBUG__
        std::stringstream ss;
        ss << "[InputHandler] Ignoring duplicate key binding:"
           << "<n>" << name << "</n>";
        DLOGW(ss.str(), "input");
#endif
        return;
    }

    key_bindings_.insert(std::make_pair(name,
        KeyBindingProperties(cooldown, trigger, key, repeat)));
}

bool InputHandler::stroke_debounce(AbstractContext& context,
                                   hash_t binding_name)
{
    auto&& kb = key_bindings_.at(binding_name);
    auto evt = context.get_key_state(kb.key);
    if(evt == kb.trigger)
    {
        if(ready(binding_name))
        {
            post("input.keyboard"_h, KbdData(binding_name));
            hot(binding_name);
            return true;
        }
        if(!kb.repeat)
            hot(binding_name);
    }
    if(evt == W_KEY_RELEASE)
    {
        cold(binding_name);
        return false;
    }
    cooldown(binding_name);
    return false;
}

void InputHandler::handle_keybindings(AbstractContext& context)
{
    for(auto&& [binding, prop]: key_bindings_)
    {
        if(stroke_debounce(context, binding))
        {
            // action map is DEPREC
            auto it = action_map_.find(binding);
            if(it != action_map_.end())
                (it->second)();
        }
    }
}

static uint8_t last_mouse_button_state = 0;
static bool last_mouse_out = false;
static bool last_mouse_locked = true;
static double last_x = 0, last_y = 0;

void InputHandler::handle_mouse(AbstractContext& context)
{
    uint8_t buttons = context.get_mouse_buttons_state();

    // Get window size
    int win_width, win_height;
    context.get_window_size(win_width, win_height);

    // Get mouse position
    double xpos, ypos;
    context.get_cursor_position(xpos, ypos);

    // Check that cursor is in window
    bool mouse_out = (xpos<0 || xpos>win_width || ypos<0 || ypos>win_height);

    // Cursor is locked -> mouse movement tracked to update camera orientation
    if(mouse_lock_)
    {
        // Reset mouse position for next frame
        context.center_cursor();

        // [HACK] (sorta) To circumvent an annoying behavior issue with GLFW where
        // the cursor position has been set but refuses to update anyhow. In this case
        // the positions returned by glfwGetCursorPos() are null. This results
        // in unwanted mouse move events during the first two frames, screwing up
        // the initial camera orientation.
        if(xpos==0.f && ypos==0.f) return;

        // Calculate deltas from last frame
        int dxi = xpos-win_width/2;
        int dyi = ypos-win_height/2;
        if(dxi!=0 || dyi!=0 || buttons!=last_mouse_button_state)
        {
            float dx = float(dxi)/win_width;
            float dy = float(dyi)/win_height;

            post("input.mouse.locked"_h, MouseData(dx, dy, buttons));
        }
    }
    // Cursor is unlocked -> edit mode
    else
    {
        // Windowed mode
        if(!GLB.SCR_FULL)
        {
            // Cursor has moved out of the window
            if(mouse_out && !last_mouse_out)
            {
                post("input.mouse.focus"_h, MouseFocusData(true));
            }
            // Cursor has moved into the window
            else if(!mouse_out && last_mouse_out)
            {
                post("input.mouse.focus"_h, MouseFocusData(false));
            }
        }
        if((!mouse_out && (xpos != last_x || ypos != last_y)) || last_mouse_locked)
            post("input.mouse.unlocked"_h, MouseData(float(xpos/GLB.WIN_W), 1.0f-float(ypos/GLB.WIN_H), buttons));
        if(buttons && (buttons!=last_mouse_button_state))
            post("input.mouse.click"_h, MouseData(float(xpos/GLB.WIN_W), 1.0f-float(ypos/GLB.WIN_H), buttons));

    }

    last_mouse_button_state = buttons;
    last_mouse_out = mouse_out;
    last_mouse_locked = mouse_lock_;
    last_x = xpos;
    last_y = ypos;
}

}
