#ifndef WCORE_H
#define WCORE_H

#include <functional>
#include <memory>

namespace wcore
{
    class Engine
    {
    public:
        Engine();
        ~Engine();

        void Init(int argc, char const *argv[],
                  std::function<void(int, char const **)> argument_parser);
        void LoadStart();
        int Run();

    private:
        struct EngineResources;
        std::unique_ptr<EngineResources> eres_; // opaque pointer
    };
}

#endif // WCORE_H
