#ifndef WCORE_H
#define WCORE_H

#include <memory>
#include <cstdint>

#include "wapi.h"
#include "wtypes.h"
#include "wecs.h"

using wcore::H_;

namespace wcore
{
    // Globals access
    extern "C" void WAPI GlobalsSet(hash_t name, const void* data);

    class WAPI Engine
    {
    public:
        Engine();
        ~Engine();

        void Init(int argc, char const *argv[], void(*parse_arguments)(int, char const **)=nullptr);
        void LoadStart();
        int Run();

    private:
        struct EngineImpl;
        std::unique_ptr<EngineImpl> eimpl_; // opaque pointer
    };
}

#endif // WCORE_H
