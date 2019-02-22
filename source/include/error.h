#ifndef ERROR_H
#define ERROR_H

#include <string>

#include "wtypes.h"

namespace wcore
{

[[noreturn]] extern void fatal(const char* message="") noexcept;
[[noreturn]] extern inline void fatal(const std::string& message) noexcept
{
    fatal(message.c_str());
}

extern void show_driver_error(const char* line="");

}

#endif // ERROR_H
