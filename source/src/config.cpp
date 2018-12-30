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

#ifdef __DEBUG__
static std::vector<std::string> LOGGER_CHANNELS
{
    "texture", "material", "model", "shader",
    "text", "input", "buffer", "chunk",
    "parsing", "entity", "scene", "io",
    "profile", "collision"
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
#endif

void Config::init()
{
    DLOGS("[Config] Beginning configuration step.", "core", Severity::LOW);
    self_path_ = get_selfpath();
    root_path_ = self_path_.parent_path().parent_path();
    DLOGI("Self path: <p>" + self_path_.string() + "</p>", "core", Severity::LOW);
    DLOGI("Root path: <p>" + root_path_.string() + "</p>", "core", Severity::LOW);

    conf_path_ = root_path_ / "config";
    if(!fs::exists(conf_path_))
    {
        DLOGE("[Config] Missing 'config' folder in root directory.", "core", Severity::CRIT);
        return;
    }
    DLOGI("Config path: <p>" + conf_path_.string() + "</p>", "core", Severity::LOW);

    DLOGN("[Config] Parsing xml configuration file.", "core", Severity::LOW);
    xml_parser_.load_file_xml(conf_path_ / "config.xml");
    retrieve_configuration(xml_parser_.get_root(), "root");

#ifdef __DEBUG__
    init_logger_channels();
#endif

    DLOGES("core", Severity::LOW);
}

// Recursive parser
void Config::retrieve_configuration(rapidxml::xml_node<>* node,
                                    const std::string& name_chain)
{
    // For each siblings at this recursion level
    for(xml_node<>* cur_node=node->first_node();
        cur_node;
        cur_node=cur_node->next_sibling())
    {
        // Look for children node if any
        xml_node<>* child_node = cur_node->first_node();
        if(child_node)
        {
            // Get current node name and append to chain
            const char* node_name = cur_node->name();
            std::string chain(name_chain + "." + node_name);
            // Get configuration for next level
            retrieve_configuration(cur_node, chain);
        }
        else
        {
            // If no child, then try to extract property
            hash_t name_hash = parse_xml_property(cur_node, name_chain);
            #ifndef __PRESERVE_STRS__
            if(name_hash)
            {
                dom_locations_.insert(std::make_pair(name_hash, cur_node));
            }
            #else
            if(name_hash.size())
            {
                dom_locations_.insert(std::make_pair(name_hash, cur_node));
            }
            #endif
            else // Node is invalid
            {
                DLOGW("[Config] Ignoring ill-formed property node.", "core", Severity::WARN);
                DLOGI("At: " + name_chain + ": " + cur_node->name(), "core", Severity::WARN);
            }
        }
    }
}

static const hash_t H_UINT   = H_("uint");
static const hash_t H_INT    = H_("int");
static const hash_t H_FLOAT  = H_("float");
static const hash_t H_STRING = H_("string");
static const hash_t H_BOOL   = H_("bool");
static const hash_t H_PATH   = H_("path");


hash_t Config::parse_xml_property(rapidxml::xml_node<>* node,
                                  const std::string& name_chain)
{
    std::string str_var_name;
    if(!xml::parse_attribute(node, "name", str_var_name))
        return 0;

    std::string str_full_name(name_chain + "." + str_var_name);
    hash_t full_name_hash = H_(str_full_name.c_str());

    // Get hash from node name
    hash_t nameHash = H_(node->name());

    switch(nameHash)
    {
        case H_UINT:
        {
            uint32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case H_INT:
        {
            int32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case H_FLOAT:
        {
            float value = 0.0f;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
        case H_STRING:
        {
            std::string value;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value.c_str());
            break;
        }
        case H_PATH:
        {
            std::string value;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            fs::path dir(root_path_ / fs::path(value.c_str()));
            if(fs::exists(dir))
                this->set_ref(full_name_hash, dir);
            else
            {
                DLOGE("[Config] Directory does not exist: ", "core", Severity::CRIT);
                DLOGI(dir.string(), "core", Severity::CRIT);
                DLOGI("Skipping.", "core", Severity::CRIT);
                return 0;
            }
            break;
        }
        case H_BOOL:
        {
            bool value = false;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            this->set(full_name_hash, value);
            break;
        }
    }

    return full_name_hash;
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

}
