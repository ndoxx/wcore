#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <functional>
#include <string>

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

        virtual std::string to_string() const {return "";}

        WID sender_;
};

struct KbdData : public WData
{
    public:
        KbdData(hash_t keyBinding): keyBinding(keyBinding) {}
        hash_t keyBinding;
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
