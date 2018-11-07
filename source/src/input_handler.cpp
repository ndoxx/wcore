#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "xml_utils.hpp"
#include "input_handler.h"
#include "logger.h"
#include "keymap.h"

using namespace rapidxml;

InputHandler::InputHandler():
mouse_lock_(true)
{

}

InputHandler::InputHandler(const char* xml_file):
mouse_lock_(true)
{
    import_key_bindings(xml_file);
}

void InputHandler::import_key_bindings(const char* xml_file)
{
    DLOGN("[InputHandler] Parsing xml file for key bindings:");
    DLOGI("<p>" + std::string(xml_file) + "</p>");

    // Read the xml file into a vector
    std::ifstream kb_file(xml_file);
    buffer_ = std::vector<char>((std::istreambuf_iterator<char>(kb_file)),
                                 std::istreambuf_iterator<char>());
    buffer_.push_back('\0');

    // Parse the buffer using the xml file parsing library into DOM
    dom_.parse<0>(&buffer_[0]);

    // Find our root node
    root_ = dom_.first_node("KeyBindings");
    if(!root_)
    {
        DLOGE("[InputHandler] No <x>KeyBindings</x> node.");
        return;
    }

    for (xml_node<>* cat=root_->first_node("Category");
         cat; cat=cat->next_sibling("Category"))
    {
        std::string category;
        xml::parse_attribute(cat, "name", category);
        DLOGI("Parsing category: <x>" + category + "</x>");

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
                DLOGW("[InputHandler] Unknown key name:");
                DLOGI(str_key);
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
#ifdef __DEBUG_KB__
                std::stringstream ss;
                ss << "[<i>" << ((trigger==GLFW_PRESS)?"PRESS":"RELEASE")
                   << "</i>] <n>" << str_key << "</n> -> " << str_desc;
                DLOG(ss.str());
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
#ifdef __DEBUG_KB__
        std::stringstream ss;
        ss << "[InputHandler] Ignoring duplicate key binding:"
           << "<n>" << name << "</n>";
        DLOGW(ss.str());
#endif
        return;
    }

    key_bindings_.insert(std::make_pair(name,
        KeyBindingProperties(cooldown, trigger, key, repeat)));
}

void InputHandler::stroke_debounce(GLFWwindow* window,
                                   hash_t binding_name,
                                   std::function<void(void)> Action)
{
    const uint16_t& key  = key_bindings_.at(binding_name).key;
    const uint16_t& trig = key_bindings_.at(binding_name).trigger;
    const bool& repeat   = key_bindings_.at(binding_name).repeat;

    auto evt = glfwGetKey(window, key);
    if(evt == trig)
    {
        if(ready(binding_name))
        {
            post(binding_name, NullData());
            Action();
            hot(binding_name);
            return;
        }
        if(!repeat)
            hot(binding_name);
    }
    if(evt == GLFW_RELEASE)
    {
        cold(binding_name);
        return;
    }
    cooldown(binding_name);
}

void InputHandler::register_action(hash_t binding_name,
                                   std::function<void(void)> Action)
{
    auto it = key_bindings_.find(binding_name);
    if(it != key_bindings_.end())
        action_map_.insert(std::make_pair(binding_name, Action));
    else
    {
#ifdef __DEBUG_KB__
        std::stringstream ss;
        ss << "[InputHandler] Ignoring unknown key binding:"
           << "<n>" << binding_name << "</n>";
        DLOGW(ss.str());
#endif
    }
}

void InputHandler::handle_keybindings(GLFWwindow* window)
{
    for(auto pair: action_map_)
    {
        stroke_debounce(window, pair.first, pair.second);
    }
}

void InputHandler::handle_mouse(GLFWwindow* window,
                                std::function<void(float dx, float dy)> Action)
{
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS || mouse_lock_)
    {
        if(!mouse_lock_) return;

        // Get window size
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);

        // Get mouse position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Reset mouse position for next frame
        glfwSetCursorPos(window,
                         win_width/2,
                         win_height/2);


        // Compute new orientation
        float dx = float(xpos-win_width/2)/win_width;
        float dy = float(ypos-win_height/2)/win_height;

        Action(dx,dy);
    }
}
