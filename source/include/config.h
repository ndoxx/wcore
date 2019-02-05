#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <vector>
#include <filesystem>
#include <vector>

#include "singleton.hpp"
#include "value_map.h"
#include "wtypes.h"
#include "xml_parser.h"

namespace wcore
{

namespace fs = std::filesystem;

// Singleton class for holding named global variables
class Config: public Singleton<Config>, public ValueMap
{
private:
    // Paths
    fs::path self_path_;
    fs::path conf_path_;

    Config (const Config&){}
    Config(){}
   ~Config(){}

public:
    friend Config& Singleton<Config>::Instance();
    friend void Singleton<Config>::Kill();

    inline const fs::path& get_root_directory() const   { return root_path_; }
    inline const fs::path& get_config_directory() const { return conf_path_; }

    // Initialize directory info
    void init();
#ifdef __DEBUG__
    void init_logger_channels();
    // Display maps content
    void debug_display_content();
#endif // __DEBUG__
};

#define CONFIG Config::Instance()

}

#endif // CONFIG_H
