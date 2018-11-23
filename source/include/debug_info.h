#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H

#include <cstdint>
#include <string>
#include <map>

#include "singleton.hpp"
#include "math3d.h"
#include "utils.h"

namespace wcore
{

class TextRenderer;
class DebugInfo : public Singleton<DebugInfo>
{
private:
    DebugInfo (const DebugInfo&)=delete;
    DebugInfo();
   ~DebugInfo()=default;

   TextRenderer* text_renderer_;
   bool active_;

   std::map<hash_t, uint8_t> slots_;
   std::map<hash_t, math::vec3> colors_;

public:
    friend DebugInfo& Singleton<DebugInfo>::Instance();
    friend void Singleton<DebugInfo>::Kill();

    void register_text_renderer(TextRenderer* prenderer);
    void register_text_slot(hash_t slot_name, const math::vec3& color);
    void display(uint8_t index, const std::string& text, const math::vec3& color);

    inline void display(hash_t slot_name, const std::string& text);
    inline void toggle() { active_=!active_; }
    inline bool active() const { return active_; }
};

inline void DebugInfo::display(hash_t slot_name, const std::string& text)
{
    display(slots_.at(slot_name), text, colors_.at(slot_name));
}


#define DINFO DebugInfo::Instance()

}

#endif // DEBUG_INFO_H
