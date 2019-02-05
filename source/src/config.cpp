#ifdef __linux__
    #include <unistd.h>
    #include <climits>
#elif _WIN32

#endif

#include "config.h"
#include "xml_utils.hpp"
#include "logger.h"

namespace wcore
{

using namespace rapidxml;

// Get path to executable
static fs::path get_selfpath()
{
#ifdef __linux__
    char buff[PATH_MAX];
    std::size_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if (len != -1)
    {
        buff[len] = '\0';
        return fs::path(buff);
    }
    else
    {
        DLOGE("Cannot read self path using readlink.", "core", Severity::CRIT);
        return fs::path();
    }
#elif _WIN32

    DLOGE("get_selfpath() not yet implemented.", "core", Severity::CRIT);
    return fs::path();

#endif
}

void Config::init()
{
    DLOGS("[Config] Beginning configuration step.", "core", Severity::LOW);
    // Get path to executable
    self_path_ = get_selfpath();
    // Deduce path to root directory
    set_root_directory(self_path_.parent_path().parent_path());

    DLOGI("Self path: <p>" + self_path_.string() + "</p>", "core", Severity::LOW);
    DLOGI("Root path: <p>" + root_path_.string() + "</p>", "core", Severity::LOW);

    // Deduce path to config folder
    conf_path_ = root_path_ / "config";
    if(!fs::exists(conf_path_))
    {
        DLOGE("[Config] Missing 'config' folder in root directory.", "core", Severity::CRIT);
        return;
    }
    DLOGI("Config path: <p>" + conf_path_.string() + "</p>", "core", Severity::LOW);

    DLOGN("[Config] Parsing xml configuration file.", "core", Severity::LOW);
    parse_xml_file(conf_path_ / "config.xml");

#ifdef __DEBUG__
    init_logger_channels();
    // Also, setup backtrace printing behavior
    bool backtrace_on_error = false;
    get("root.debug.logger.print_backtrace_on_error"_h,  backtrace_on_error);
    dbg::LOG.set_backtrace_on_error(backtrace_on_error);
#endif

    DLOGES("core", Severity::LOW);
}

#ifdef __DEBUG__
static std::vector<std::string> LOGGER_CHANNELS
{
    "texture", "material",  "model",  "shader",
    "text",    "input",     "buffer", "chunk",
    "parsing", "entity",    "scene",  "io",
    "profile", "collision", "sound",  "editor"
};

void Config::init_logger_channels()
{
    for(auto&& channel: LOGGER_CHANNELS)
    {
        uint32_t verbosity = 0u;
        get(H_(("root.debug.channel_verbosity."+channel).c_str()),  verbosity);
        dbg::LOG.register_channel(channel.c_str(),  verbosity);
    }
}

void Config::debug_display_content()
{
    std::cout << "--uints--" << std::endl;
    for(auto&& [key, value]: uints_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--ints--" << std::endl;
    for(auto&& [key, value]: ints_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--floats--" << std::endl;
    for(auto&& [key, value]: floats_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--strings--" << std::endl;
    for(auto&& [key, value]: strings_)
        std::cout << "    " << key << " -> " << value << std::endl;

    std::cout << "--bools--" << std::endl;
    for(auto&& [key, value]: bools_)
        std::cout << "    " << key << " -> " << value << std::endl;
}
#endif

}
