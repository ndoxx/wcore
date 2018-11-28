#ifndef WCORE_H
#define WCORE_H

#include <memory>
#include <cstdint>

#include "wapi.h"
#include "wtypes.h"

using wcore::H_;
using wcore::HS_;

namespace wcore
{
    // Globals access
    extern "C" void WEXPORT GlobalsSet(hashstr_t name, const void* data);

    class WEXPORT Engine
    {
    public:
        Engine();
        ~Engine();

        void Init(int argc, char const *argv[], void(*parse_arguments)(int, char const **));
        void LoadStart();
        int Run();

    private:
        struct EngineImpl;
        std::unique_ptr<EngineImpl> eimpl_; // opaque pointer
    };
}

#endif // WCORE_H
