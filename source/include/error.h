#ifndef ERROR_H
#define ERROR_H

#include <string>

#include "utils.h"

namespace wcore
{

extern void fatal(const char* message="");
extern inline void fatal(const std::string& message)
{
    fatal(message.c_str());
}

}

#endif // ERROR_H
