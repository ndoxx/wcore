#ifndef COMPONENT_DETAIL_H
#define COMPONENT_DETAIL_H

#include <unordered_map>
#include <typeindex>

#include "logger.h"

namespace wcore
{

/* Original idea by Paul Manta on:
 * http://gamedev.stackexchange.com/questions/17746/entity-component-systems-in-c-how-do-i-discover-types-and-construct-component
 */

class WComponent;
namespace component
{
namespace detail
{
    typedef WComponent* (*CreateComponentFunc)();
    typedef std::unordered_map<std::type_index, CreateComponentFunc> ComponentRegistry;

    inline ComponentRegistry& getComponentRegistry()
    {
        static ComponentRegistry reg;
        return reg;
    }

    //TODO: TEMPLATE THIS WITH ALLOCATOR TYPE, WE WANT TO ENSURE BLOCK ALLOCATION!!!
    template<class T>
    WComponent* createComponent()
    {
        return new T;
    }

    template<class T>
    struct RegistryEntry
    {
    public:
        static RegistryEntry<T>& Instance(std::type_index type)
        {
            // Because I use a singleton here, even though `COMPONENT_REGISTER`
            // is expanded in multiple translation units, the constructor
            // will only be executed once. Only this cheap `Instance` function
            // (which most likely gets inlined) is executed multiple times.

            static RegistryEntry<T> inst(type);
            return inst;
        }

    private:
        RegistryEntry(std::type_index type)
        {
            ComponentRegistry& reg = getComponentRegistry();
            CreateComponentFunc func = createComponent<T>;

            std::pair<ComponentRegistry::iterator, bool> ret =
                reg.insert(ComponentRegistry::value_type(type, func));

            if (ret.second == false)
            {
                // This means there already is a component registered to
                // this name.
                DLOGW("Commponent already defined: ", "entity", Severity::WARN);
            }
        }

        RegistryEntry(const RegistryEntry<T>&)            = delete;
        RegistryEntry& operator=(const RegistryEntry<T>&) = delete;
    };

} // namespace detail
} // namespace component
} // namespace wcore
#endif // COMPONENT_DETAIL_H
