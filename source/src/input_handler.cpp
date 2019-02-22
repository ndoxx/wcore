#include <GL/glew.h>

#include "xml_utils.hpp"
#include "input_handler.h"
#include "logger.h"
#include "keymap.h"
#include "context.h"
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
        DLOGE("[InputHandler] Unable to open file:", "input", Severity::CRIT);
        DLOGI(keybindingsfile, "input", Severity::CRIT);
        fatal();
    }
    xml_parser_.load_file_xml(*pstream);

    for (xml_node<>* cat=xml_parser_.get_root()->first_node("Category");
         cat; cat=cat->next_sibling("Category"))
    {
        std::string category;
        xml::parse_attribute(cat, "name", category);
        DLOGI("Parsing category: <x>" + category + "</x>", "input", Severity::LOW);

        for (xml_node<>* kb=cat->first_node("KB");
             kb; kb=kb->next_sibling("KB"))
        {
            uint16_t cooldown = 0,
                     trigger  = GLFW_PRESS,
                     key      = 0;
            bool repeat = false;
            std::string str_name, str_key, str_trigger;

            // Mandatory attributes
            if(!xml::parse_attribute(kb, "name", str_name) ||
               !xml::parse_attribute(kb, "key", str_key))
                continue;

            // Lookup GLFW key value
            auto it = keymap::NAMES.find(H_(str_key.c_str()));
            if(it == keymap::NAMES.end())
            {
                DLOGW("[InputHandler] Unknown key name:", "input", Severity::WARN);
                DLOGI(str_key, "input", Severity::WARN);
                continue;
            }
            key = it->second;

            // Optional attributes
            xml::parse_attribute(kb, "cooldown", cooldown);
            xml::parse_attribute(kb, "repeat", repeat);
            if(xml::parse_attribute(kb, "trigger", str_trigger))
            {
                if(!str_trigger.compare("release"))
                    trigger = GLFW_RELEASE;
                else
                    trigger  = GLFW_PRESS;
            }

            std::string str_desc;
            if(xml::parse_attribute(kb, "description", str_desc))
            {
#ifdef __DEBUG__
                std::stringstream ss;
                ss << "[<i>" << ((trigger==GLFW_PRESS)?"PRESS":"RELEASE")
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
        DLOGW(ss.str(), "input", Severity::WARN);
#endif
        return;
    }

    key_bindings_.insert(std::make_pair(name,
        KeyBindingProperties(cooldown, trigger, key, repeat)));
}

bool InputHandler::stroke_debounce(Context& context,
                                   hash_t binding_name)
{
    auto && kb = key_bindings_.at(binding_name);
    auto evt = glfwGetKey(context.window_, kb.key);
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
    if(evt == GLFW_RELEASE)
    {
        cold(binding_name);
        return false;
    }
    cooldown(binding_name);
    return false;
}

// deprec
bool InputHandler::stroke_debounce(Context& context,
                                   hash_t binding_name,
                                   std::function<void(void)> Action)
{
    auto && kb = key_bindings_.at(binding_name);
    auto evt = glfwGetKey(context.window_, kb.key);
    if(evt == kb.trigger)
    {
        if(ready(binding_name))
        {
            post("input.keyboard"_h, KbdData(binding_name));
            Action();
            hot(binding_name);
            return true;
        }
        if(!kb.repeat)
            hot(binding_name);
    }
    if(evt == GLFW_RELEASE)
    {
        cold(binding_name);
        return false;
    }
    cooldown(binding_name);
    return false;
}

// deprec
void InputHandler::register_action(hash_t binding_name,
                                   std::function<void(void)> Action)
{
    auto it = key_bindings_.find(binding_name);
    if(it != key_bindings_.end())
        action_map_.insert(std::make_pair(binding_name, Action));
    else
    {
#ifdef __DEBUG__
        std::stringstream ss;
        ss << "[InputHandler] Ignoring unknown key binding:"
           << "<n>" << binding_name << "</n>";
        DLOGW(ss.str(), "input", Severity::WARN);
#endif
    }
}

void InputHandler::handle_keybindings(Context& context)
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

void InputHandler::handle_mouse(Context& context)
{
    uint8_t buttons = (glfwGetMouseButton(context.window_, GLFW_MOUSE_BUTTON_LEFT)   << MouseButton::LMB)
                    + (glfwGetMouseButton(context.window_, GLFW_MOUSE_BUTTON_RIGHT)  << MouseButton::RMB)
                    + (glfwGetMouseButton(context.window_, GLFW_MOUSE_BUTTON_MIDDLE) << MouseButton::MMB);

    // Get window size
    int win_width, win_height;
    glfwGetWindowSize(context.window_, &win_width, &win_height);

    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(context.window_, &xpos, &ypos);

    // Check that cursor is in window
    bool mouse_out = (xpos<0 || xpos>win_width || ypos<0 || ypos>win_height);

    // Cursor is locked -> mouse movement tracked to update camera orientation
    if(mouse_lock_)
    {
        // Reset mouse position for next frame
        glfwSetCursorPos(context.window_,
                         win_width/2,
                         win_height/2);


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
