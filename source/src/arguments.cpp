#include <string>
#include <algorithm>

#include "arguments.h"
#include "config.h"
#include "globals.h"

namespace wcore
{

static const char* get_cmd_option(const char** begin, const char ** end, const std::string & option)
{
    const char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

static bool cmd_option_exists(const char** begin, const char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

void parse_program_arguments(int argc, char const *argv[])
{
    // First, try to initialize default using config
    CONFIG.get(H_("root.display.width"),  GLB.SCR_W);
    CONFIG.get(H_("root.display.height"), GLB.SCR_H);
    CONFIG.get(H_("root.display.full"),   GLB.SCR_FULL);

    // Screen size specified in arguments? Initialize screen size accordingly.
    const char* screenSize = get_cmd_option(argv, argv + argc, "-s");
    if(screenSize)
    {
        std::string screenSizeStr(screenSize);
        std::size_t xpos = screenSizeStr.find("x");
        GLB.SCR_W = std::stoi(screenSizeStr.substr(0, xpos));
        GLB.SCR_H = std::stoi(screenSizeStr.substr(xpos+1));
    }

    // Fullscreen
    if(cmd_option_exists(argv, argv + argc, "-f"))
        GLB.SCR_FULL = true;
}

}
