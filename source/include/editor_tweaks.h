#ifndef EDITOR_TWEAKS_H
#define EDITOR_TWEAKS_H

#include "game_system.h"

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
};

} // namespace wcore

#endif // EDITOR_TWEAKS_H
