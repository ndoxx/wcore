#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <functional>
#include <string>
#include <sstream>
#include <bitset>
#include <iomanip>

#include "utils.h"

namespace wcore
{

typedef size_t WID;
typedef size_t WDelegateID;

struct WData
{
    public:
        WData(): WData(0){}
        WData(WID sender_id): sender_(sender_id){}
        virtual ~WData(){}

#ifdef __DEBUG__
        virtual std::string to_string() const {return "";}
#endif

        WID sender_;
};

struct KbdData : public WData
{
    public:
        KbdData(hash_t keyBinding): key_binding(keyBinding) {}
        hash_t key_binding;
};

// Offsets in bitset
enum MouseButton : uint8_t
{
    LMB  = 0,   // Left mouse button
    RMB  = 1,   // Right mouse button
    MMB  = 2,   // Middle mouse button
};

struct MouseData : public WData
{
    MouseData(float dx, float dy, uint8_t pressed=0):
    dx(dx),
    dy(dy),
    button_pressed(pressed)
    {

    }

#ifdef __DEBUG__
    virtual std::string to_string() const override
    {
        std::stringstream ss;
        ss << std::setprecision(5) << std::fixed
           << "dx= " << dx << " dy= " << dy
           << " p: " << button_pressed;
        return ss.str();
    }
#endif

    float dx = 0.f;
    float dy = 0.f;
    std::bitset<4> button_pressed;
};

struct NullData : public WData {};

typedef std::function<void(const WData&)> WpFunc;

// Delegate creation helper functions
namespace dlg
{
    template <typename T, typename INST>
    static WpFunc make_delegate(void (T::*func)(const WData&), INST& inst)
    {
        T* ptr = static_cast<T*>(&inst);
        return std::bind(func, ptr, std::placeholders::_1);
    }

    template <typename D>
    static WpFunc make_delegate(void (*func)(const WData&))
    {
        return std::bind(func, std::placeholders::_1);
    }
}

}

#endif // TYPES_H_INCLUDED
