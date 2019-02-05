#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <vector>
#include <filesystem>
#include <vector>

#include "singleton.hpp"
#include "wtypes.h"
#include "xml_parser.h"

namespace wcore
{

namespace fs = std::filesystem;

// Singleton class for holding named global variables
class Config: public Singleton<Config>
{
private:
    // Configuration key/values
    std::map<hash_t, uint32_t>    uints_;
    std::map<hash_t, int32_t>     ints_;
    std::map<hash_t, float>       floats_;
    std::map<hash_t, std::string> strings_;
    std::map<hash_t, bool>        bools_;
    std::map<hash_t, fs::path>    paths_;

    // Paths
    fs::path self_path_;
    fs::path root_path_;
    fs::path conf_path_;

    XMLParser xml_parser_;
    std::map<hash_t, rapidxml::xml_node<>*> dom_locations_;

    Config (const Config&){}
    Config(){}
   ~Config(){}

public:
    friend Config& Singleton<Config>::Instance();
    friend void Singleton<Config>::Kill();

    // Generic accessors for values in maps
    template <typename T> inline void set(hash_t name, T value);
    template <typename T> inline void set_ref(hash_t name, const T& value);
    template <typename T> inline void set_move(hash_t name, T&& value);
    template <typename T> inline bool get(hash_t name, T& destination);
    // Test a boolean flag quickly
    inline bool is(hash_t name);

    inline const fs::path& get_root_directory() const   { return root_path_; }
    inline const fs::path& get_config_directory() const { return conf_path_; }

    // Initialize directory info
    void init();
#ifdef __DEBUG__
    void init_logger_channels();
#endif // __DEBUG__
    // Display maps content
    void debug_display_content();

private:
    // Recursive method for XML data hierarchy exploration
    void retrieve_configuration(rapidxml::xml_node<>* node,
                                const std::string& name_chain);
    // Parse a leaf value node in DOM
    hash_t parse_xml_property(rapidxml::xml_node<>* node,
                              const std::string& name_chain);
};

inline bool Config::is(hash_t name)
{
    auto it = bools_.find(name);
    if(it != bools_.end())
    {
        return it->second;
    }
    return false;
}

// Accessors specializations
template <> inline void Config::set(hash_t name, uint32_t value)
{
    uints_[name] = value;
}
template <> inline bool Config::get(hash_t name, uint32_t& destination)
{
    auto it = uints_.find(name);
    if(it != uints_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> inline void Config::set(hash_t name, int32_t value)
{
    ints_[name] = value;
}
template <> inline bool Config::get(hash_t name, int32_t& destination)
{
    auto it = ints_.find(name);
    if(it != ints_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> inline void Config::set(hash_t name, float value)
{
    floats_[name] = value;
}
template <> inline bool Config::get(hash_t name, float& destination)
{
    auto it = floats_.find(name);
    if(it != floats_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> inline void Config::set(hash_t name, const char* value)
{
    strings_[name] = std::string(value);
}
template <> inline void Config::set(hash_t name, char* value)
{
    strings_[name] = std::string(value);
}

template <> inline bool Config::get(hash_t name, std::string& destination)
{
    auto it = strings_.find(name);
    if(it != strings_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> inline void Config::set_ref(hash_t name, const fs::path& value)
{
    paths_[name] = value;
}

template <> inline void Config::set_move(hash_t name, fs::path&& value)
{
    paths_[name] = std::move(value);
}

template <> inline bool Config::get(hash_t name, fs::path& destination)
{
    auto it = paths_.find(name);
    if(it != paths_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

template <> inline void Config::set(hash_t name, bool value)
{
    bools_[name] = value;
}
template <> inline bool Config::get(hash_t name, bool& destination)
{
    auto it = bools_.find(name);
    if(it != bools_.end())
    {
        destination = it->second;
        return true;
    }
    return false;
}

#define CONFIG Config::Instance()

}

#endif // CONFIG_H
