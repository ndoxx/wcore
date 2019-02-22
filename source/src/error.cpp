#include <cstdlib>
#include <iostream>
#include <sstream>

#include "error.h"
#include "gfx_driver.h"
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
    auto error = GFX::get_error();

    std::stringstream ss;
    ss << line << ": " << std::to_string(error);

    if(error)
        DLOGB(ss.str(), "core", Severity::CRIT);
    else
        DLOGG(ss.str(), "core", Severity::LOW);
}


}
