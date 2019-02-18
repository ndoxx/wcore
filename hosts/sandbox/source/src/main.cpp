#include "wcore.h"
#include "arguments.h"

using namespace wcore;

int main(int argc, char const *argv[])
{
    wcore::Engine engine;
    engine.Init(argc, argv, sandbox::parse_program_arguments);
    engine.UseResourceArchive("pack0.zip", "pack0"_h);
    engine.LoadStart();
    return engine.Run();
}
