#include <string>
#include <algorithm>

#include "arguments.h"
#include "config.h"
#include "globals.h"

namespace rd_test
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
    wcore::CONFIG.get(wcore::HS_("root.display.width"),  wcore::GLB.SCR_W);
    wcore::CONFIG.get(wcore::HS_("root.display.height"), wcore::GLB.SCR_H);
    wcore::CONFIG.get(wcore::HS_("root.display.full"),   wcore::GLB.SCR_FULL);

    // Screen size specified in arguments? Initialize screen size accordingly.
    const char* screenSize = get_cmd_option(argv, argv + argc, "-s");
    if(screenSize)
    {
        std::string screenSizeStr(screenSize);
        std::size_t xpos = screenSizeStr.find("x");
        wcore::GLB.SCR_W = std::stoi(screenSizeStr.substr(0, xpos));
        wcore::GLB.SCR_H = std::stoi(screenSizeStr.substr(xpos+1));
    }

    // Level name specified?
    const char* levelName = get_cmd_option(argv, argv + argc, "-l");
    if(levelName)
    {
        wcore::GLB.START_LEVEL = levelName;
    }

    // Fullscreen
    if(cmd_option_exists(argv, argv + argc, "-f"))
        wcore::GLB.SCR_FULL = true;
}

}
