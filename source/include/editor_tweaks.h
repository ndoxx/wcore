#ifndef EDITOR_TWEAKS_H
#define EDITOR_TWEAKS_H

#include <functional>
#include <iostream>
#include <map>

#include "game_system.h"
#include "value_map.h"

namespace wcore
{

class EditorTweaksInitializer: public InitializerSystem
{
public:
    // * Override
    virtual ~EditorTweaksInitializer();
    // Parse tweaks file and initialize state
    virtual void init_self() override;
    // Write to tweaks file to save state
    virtual void serialize() override;

    // * Methods
    // Register a variable to be automatically deserialized/serialized at engine start/stop
    template <typename T>
    void register_variable(hash_t name, T& destination)
    {
        value_map_.get(name, destination);

        serializers_.insert(std::pair(name, [=, &destination]()
        {
            value_map_.set<T>(name, destination, true);
        }));
    }

private:
    ValueMap value_map_;
    std::map<hash_t, std::function<void()>> serializers_;
};

} // namespace wcore

#endif // EDITOR_TWEAKS_H
