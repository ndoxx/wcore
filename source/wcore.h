#ifndef WCORE_H
#define WCORE_H

#include <memory>

#include "wapi.h"

namespace wcore
{
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
