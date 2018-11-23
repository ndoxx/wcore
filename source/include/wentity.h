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
            std::stringstream ss;
            ss << "Ignoring duplicate component: " << typeid(T).name();
            DLOGW(ss.str(), "entity", Severity::WARN);
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
};

}

#endif // WENTITY_H
