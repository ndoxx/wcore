#include "wcore.h"
#include "arguments.h"

int main(int argc, char const *argv[])
{
    wcore::Engine engine;
    engine.Init(argc, argv, sandbox::parse_program_arguments);
    engine.LoadStart();
    return engine.Run();
}
