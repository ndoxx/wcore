#include <cstdlib>
#include <iostream>

#include "error.h"

namespace wcore
{

void fatal(const char* message)
{
    std::cerr << "[FATAL ERROR] " << message << std::endl;
    exit(EXIT_FAILURE);
}

}
