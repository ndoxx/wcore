#include <cstdlib>
#include <iostream>
#include <sstream>

#include "error.h"
#include "gfx_api.h"
#include "logger.h"

namespace wcore
{

void fatal(const char* message) noexcept
{
    std::cerr << "[FATAL ERROR] " << message << std::endl;
    exit(EXIT_FAILURE);
}

void show_driver_error(const char* line)
{
    auto error = Gfx::get_error();

    std::stringstream ss;
    ss << line << ": " << std::to_string(error);

    if(error)
        DLOGB(ss.str(), "core");
    else
        DLOGG(ss.str(), "core");
}


}
