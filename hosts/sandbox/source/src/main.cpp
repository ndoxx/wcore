#include <thread>

#include "wcore.h"
#include "arguments.h"

int main(int argc, char const *argv[])
{
    wcore::Engine engine;
    engine.Init(argc, argv, sandbox::parse_program_arguments);
    engine.scene->LoadStart();
    return engine.Run();

    /*while(engine.WindowRequired())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(5000));
        engine.Update(16.67/1000.f);
        engine.RenderFrame();
        engine.FinishFrame();
    }*/

    return 0;
}
