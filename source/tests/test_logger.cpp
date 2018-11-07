#include "logger.h"

int main(int argc, char const *argv[])
{
    Logger::Instance().print_reference();

    std::cout << std::endl << std::endl;

    DLOGN("File: <p>../res/textures/poopsie.png</p> of size <v>26MB</v>");
    DLOGI("Serves <b>no</b> purpose.");
    DLOGI("It will be <i>corrupted</i> automatically in <v>10</v>s.");

    DLOGN("Trying to load <g>wondrous</g> asset: <n>poopsie</n>");
    DLOGW("File: <p>../res/textures/poopsie.png</p> is <b>corrupted</b>.");
    DLOGN("Trying to load <d>DEFAULT</d> asset instead.");
    DLOGE("Error trying to read the file: <p>../res/textures/poopsie.png</p>");
    DLOGF("Fatal error occurred while displaying error message.");
    BANG();

    return 0;
}
