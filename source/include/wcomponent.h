#ifndef WCOMPONENT_H
#define WCOMPONENT_H

#include <map>
#include <utility>
#include <memory>
#include <cassert>

#include "wcomponent_detail.h"

class WEntity;
class WComponent
{
public:
    WComponent(): parent_(nullptr), active_(true) {}
    virtual ~WComponent() = 0; // Pure virtual with body -> class is abstract
                               // but derived classes' default destructors
                               // can still chain-up.
    WEntity* parent_;
    bool     active_;
};

namespace component
{
    template <typename T>
    T* create()
    {
        detail::ComponentRegistry& reg = detail::getComponentRegistry();
        detail::ComponentRegistry::iterator it = reg.find(std::type_index(typeid(T)));

        // Make sure this component type is registered
        assert(it != reg.end());

        detail::CreateComponentFunc func = it->second;
        return (T*)func();
    }

    void destroy(const WComponent* comp);
}

// Macro to register a pair Type/Name for a component in the registry
// and create a factory function for it
#define REGISTER_COMPONENT(TYPE, NAME)                                        \
    namespace component {                                                     \
    namespace detail {                                                        \
    namespace                                                                 \
    {                                                                         \
        template<class T>                                                     \
        class ComponentRegistration;                                          \
                                                                              \
        template<>                                                            \
        class ComponentRegistration<TYPE>                                     \
        {                                                                     \
            static const ::component::detail::RegistryEntry<TYPE>& reg;       \
        };                                                                    \
                                                                              \
        const ::component::detail::RegistryEntry<TYPE>&                       \
            ComponentRegistration<TYPE>::reg =                                \
                ::component::detail::RegistryEntry<TYPE>::Instance(NAME);     \
    }}}

#endif // WCOMPONENT_H

