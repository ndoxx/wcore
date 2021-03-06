#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <string>

#ifdef __linux__
    #include <unistd.h>
    #include <climits>
#elif _WIN32

#endif

#include "logger.h"
#include "wtypes.h"

using namespace wcore;
namespace fs = std::filesystem;

static fs::path self_path_;
static fs::path root_path_;
static fs::path inc_path_;
static fs::path src_path_;
static fs::path conf_path_;

// Non greedy regex that matches the H_("any_str") macro
static std::regex hash_str_tag("H_\\(\"([a-zA-Z0-9_\\.]+?)\"\\)");
// Non greedy regex that matches the "abcd"_h string literal
// BUG: seems to be greedy anyway -> will generate a faulty XML file
static std::regex hash_str_literal_tag("\"([a-zA-Z0-9_\\.]+?)\"_h");
// Associates hashes to original strings
static std::map<hash_t, std::string> intern_strings_;

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
        DLOGE("Cannot read self path using readlink.", "core");
        return fs::path();
    }
#elif _WIN32

    DLOGE("get_selfpath() not yet implemented.", "core");
    return fs::path();

#endif
}

static void register_intern_string(const std::string& intern)
{
    hash_t hash_intern = H_(intern.c_str()); // Hashed string

    auto it = intern_strings_.find(hash_intern);
    if(it == intern_strings_.end())
    {
        DLOGI("<v>" + std::to_string(hash_intern) + "</v> -> " + intern, "core");
        intern_strings_.insert(std::make_pair(hash_intern, intern));
    }
    else if(it->second.compare(intern)) // Detect hash collision
    {
        DLOGW("Hash collision detected:", "core");
        DLOGI(it->second + " -> " + std::to_string(it->first), "core");
        DLOGI(intern + " -> " + std::to_string(hash_intern), "core");
        do
        {
            std::cout << '\n' << "Press ENTER to continue...";
        } while (std::cin.get() != '\n');
    }
}

// Parse a single file for hash macros
static void parse_entry(const fs::directory_entry& entry)
{
    // * Copy file to string
    DLOG("<p>" + entry.path().string() + "</p>", "core", Severity::LOW);
    std::ifstream ifs(entry.path());
    if(!ifs.is_open())
    {
        DLOGE("Unable to open file. Skipping.", "core");
        return;
    }
    std::string source_str((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());


    // * Match string hash macros and update table
    {
        std::regex_iterator<std::string::iterator> it(source_str.begin(), source_str.end(), hash_str_tag);
        std::regex_iterator<std::string::iterator> end;

        while(it != end)
        {
            std::string intern((*it)[1]); // The intern string
            register_intern_string(intern);
            ++it;
        }
    }
    {
        std::regex_iterator<std::string::iterator> it(source_str.begin(), source_str.end(), hash_str_literal_tag);
        std::regex_iterator<std::string::iterator> end;

        while(it != end)
        {
            std::string intern((*it)[1]); // The intern string
            register_intern_string(intern);
            ++it;
        }
    }
}

int main()
{
    DLOGN("Intern string utilitary launched.", "core");

    // * Locate executable path, root directory, config directory and source directories
    DLOGS("Locating sources.", "core", Severity::LOW);
    self_path_ = get_selfpath();
    root_path_ = self_path_.parent_path().parent_path();

    DLOGI("Self path: <p>" + self_path_.string() + "</p>", "core");
    DLOGI("Root path: <p>" + root_path_.string() + "</p>", "core");

    inc_path_ = root_path_ / "source" / "include";
    src_path_ = root_path_ / "source" / "src";
    conf_path_ = root_path_ / "config";


    if(!fs::exists(inc_path_))
    {
        DLOGE("Unable to locate include path.", "core");
        return -1;
    }
    if(!fs::exists(src_path_))
    {
        DLOGE("Unable to locate source path.", "core");
        return -1;
    }
    if(!fs::exists(conf_path_))
    {
        DLOGE("Unable to locate config path.", "core");
        return -1;
    }

    DLOGI("Include path: <p>" + inc_path_.string() + "</p>", "core");
    DLOGI("Source path: <p>" + src_path_.string() + "</p>", "core");
    DLOGES("core", Severity::LOW);


    DLOGS("Parsing source code for hash string occurrences.", "core", Severity::LOW);
    for(const auto& entry: fs::directory_iterator(inc_path_))
        parse_entry(entry);
    for(const auto& entry: fs::directory_iterator(src_path_))
        parse_entry(entry);
    DLOGES("core", Severity::LOW);


    // * Write intern string table to XML file
    fs::path xml_path = conf_path_ / "dbg_intern_strings.xml";
    DLOGS("Exporting intern string table to XML file.", "core", Severity::LOW);
    DLOGI(xml_path.string(), "core");
    std::ofstream out_xml(xml_path);
    if(out_xml.is_open())
    {
        out_xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
        out_xml << "<InternStrings>" << std::endl;

        for(auto&& [key,value]: intern_strings_)
        {
            out_xml << "    <string key=\"" << key << "\" value=\"" << value << "\"/>" << std::endl;
        }
        out_xml << "</InternStrings>" << std::endl;
        out_xml.close();
    }
    else
    {
        DLOGE("Unable to open output XML file.", "core");
    }
    DLOGG("Done.", "core");
    DLOGES("core", Severity::LOW);
    return 0;
}
