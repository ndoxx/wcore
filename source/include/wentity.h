#ifndef WENTITY_H
#define WENTITY_H

#include <unordered_map>
#include <typeindex>
#include <string>
#include <sstream>

#include "wcomponent.h"

namespace wcore
{

class WEntity
{
public:
    WEntity();
    virtual ~WEntity();

    template <typename T>
    T* add_component()
    {
        std::type_index index(typeid(T));
        auto it = components_.find(index);
        if(it != components_.end())
        {
            #ifdef __DEBUG__
                warn_duplicate_component(typeid(T).name());
            #endif
            return static_cast<T*>(it->second);
        }

        T* component = component::create<T>();
        component->parent_ = this;
        components_[index] = component;
        return component;
    }

    template <typename T>
    T* get_component()
    {
        std::type_index index(typeid(T));
        auto it = components_.find(index);

        return (it != components_.end())?
               static_cast<T*>(it->second)
               : nullptr;
    }

    template <typename T>
    bool has_component()
    {
        std::type_index index(typeid(T));
        return (components_.find(index) != components_.end());
    }

protected:
    std::unordered_map<std::type_index, WComponent*> components_;

private:
#ifdef __DEBUG__
    static void warn_duplicate_component(const char* name);
#endif
};

}

#endif // WENTITY_H
