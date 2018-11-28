#ifndef ERROR_H
#define ERROR_H

#include <string>

#include "wtypes.h"

namespace wcore
{

extern void fatal(const char* message="");
extern inline void fatal(const std::string& message)
{
    fatal(message.c_str());
}

extern void show_driver_error(const char* line="");

}

#endif // ERROR_H
