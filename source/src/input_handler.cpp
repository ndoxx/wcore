#include <GL/glew.h>

#include "xml_utils.hpp"
#include "input_handler.h"
#include "io_utils.h"
#include "logger.h"
#include "keymap.h"
#include "context.h"

namespace wcore
{

using namespace rapidxml;

InputHandler::InputHandler():
last_mouse_button_state_(0),
mouse_lock_(true)
{
    import_key_bindings();
}

void InputHandler::import_key_bindings()
{
    DLOGS("[InputHandler] Parsing key bindings.", "input", Severity::LOW);

    fs::path file_path(io::get_file(H_("root.folders.config"), "keybindings.xml"));
    xml_parser_.load_file_xml(file_path);

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
            post(H_("input.keyboard"), KbdData(binding_name));
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

// DEPREC
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
            post(H_("input.keyboard"), KbdData(binding_name));
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

// DEPREC
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

void InputHandler::handle_mouse(Context& context)
{
    uint8_t buttons = (glfwGetMouseButton(context.window_, GLFW_MOUSE_BUTTON_LEFT)   << MouseButton::LMB)
                    + (glfwGetMouseButton(context.window_, GLFW_MOUSE_BUTTON_RIGHT)  << MouseButton::RMB)
                    + (glfwGetMouseButton(context.window_, GLFW_MOUSE_BUTTON_MIDDLE) << MouseButton::MMB);

    if(mouse_lock_)
    {
        // Get window size
        int win_width, win_height;
        glfwGetWindowSize(context.window_, &win_width, &win_height);

        // Get mouse position
        double xpos, ypos;
        glfwGetCursorPos(context.window_, &xpos, &ypos);

        // Reset mouse position for next frame
        glfwSetCursorPos(context.window_,
                         win_width/2,
                         win_height/2);


        // Compute new orientation
        int dxi = xpos-win_width/2;
        int dyi = ypos-win_height/2;
        if(dxi!=0 || dyi!=0 || buttons!=last_mouse_button_state_)
        {
            float dx = float(dxi)/win_width;
            float dy = float(dyi)/win_height;

            post(H_("input.mouse"), MouseData(dx, dy, buttons));
        }
    }

    last_mouse_button_state_ = buttons;
}

}
