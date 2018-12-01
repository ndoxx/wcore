#include <string>
#include <algorithm>

#include "arguments.h"
#include "wcore.h"

namespace sandbox
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
    // Screen size specified in arguments? Initialize screen size accordingly.
    const char* screenSize = get_cmd_option(argv, argv + argc, "-s");
    if(screenSize)
    {
        std::string screenSizeStr(screenSize);
        std::size_t xpos = screenSizeStr.find("x");
        uint32_t scrw = std::stoi(screenSizeStr.substr(0, xpos));
        uint32_t scrh = std::stoi(screenSizeStr.substr(xpos+1));
        wcore::GlobalsSet(HS_("SCR_W"), &scrw);
        wcore::GlobalsSet(HS_("SCR_H"), &scrh);
    }

    // Level name specified?
    const char* levelName = get_cmd_option(argv, argv + argc, "-l");
    if(levelName)
    {
        wcore::GlobalsSet(HS_("START_LEVEL"), levelName);
    }

    // Fullscreen
    if(cmd_option_exists(argv, argv + argc, "-f"))
    {
        bool fullscr = true;
        wcore::GlobalsSet(HS_("SCR_FULL"), &fullscr);
    }
}

}
