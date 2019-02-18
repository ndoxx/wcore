#include <fstream>
#include <memory>
#include <map>

#include "io_utils.h"
#include "config.h"
#include "logger.h"

#include "vendor/zipios/zipfile.hpp"

namespace wcore
{
namespace io
{

fs::path get_file(hash_t folder_node, const fs::path& file_name)
{
    fs::path file_path;
    if(!CONFIG.get(folder_node, file_path))
    {
        DLOGE("Missing folder config node: <n>" + std::to_string(folder_node) + "</n>", "io", Severity::CRIT);
        return fs::path();
    }

    file_path /= file_name;
    if(!fs::exists(file_path))
    {
        DLOGE("File not found: <p>" + file_name.string() + "</p>", "io", Severity::CRIT);
        return fs::path();
    }

    return file_path;
}

std::string get_file_as_string(hash_t folder_node, const fs::path& file_name)
{
    fs::path file_path = get_file(folder_node, file_name);
    std::ifstream ifs(file_path);

    if(!ifs.is_open())
    {
        DLOGE("Unable to open file:", "io", Severity::CRIT);
        DLOGI("node= " + std::to_string(folder_node), "io", Severity::CRIT);
        DLOGI("file name= " + file_name.string(), "io", Severity::CRIT);
        DLOGI("void string returned.", "io", Severity::CRIT);

        return std::string("");
    }

    // Read file to buffer
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

std::vector<char> get_file_as_vector(const fs::path& file_path)
{
    std::ifstream ifs(file_path);

    if(!ifs.is_open())
    {
        DLOGE("Unable to open file:", "io", Severity::CRIT);
        DLOGI("file name= " + file_path.string(), "io", Severity::CRIT);
        DLOGI("void vector returned.", "io", Severity::CRIT);

        return std::vector<char>();
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    return buffer;
}

std::vector<char> get_binary_file_as_vector(const fs::path& file_path)
{
    // Open file and seek to end to get byte count
    std::ifstream ifs(file_path, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    if(!ifs.is_open())
    {
        DLOGE("Unable to open binary file:", "io", Severity::CRIT);
        DLOGI("file name= " + file_path.string(), "io", Severity::CRIT);
        DLOGI("void vector returned.", "io", Severity::CRIT);

        return std::vector<char>();
    }

    std::vector<char> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    return result;
}

// ----------- WIP -----------

static std::map<hash_t, zipios::ZipFile> archives_;

void open_archive(const fs::path& file_path, hash_t key)
{
    archives_.insert(std::pair(key, zipios::ZipFile(file_path.string().c_str())));
}

void close_archive(hash_t key)
{
    auto it = archives_.find(key);
    if(it != archives_.end())
    {
        DLOGE("Cannot close unknown archive:", "io", Severity::CRIT);
        DLOGI(std::to_string(key) + " -> " + HRESOLVE(key), "io", Severity::CRIT);
    }
    else
    {
        it->second.close();
        archives_.erase(it);
    }
}
/*
bool get_file_as_stream(const fs::path& file_path, std::istream& stream)
{
    std::ifstream ifs(file_path);

    if(!ifs.is_open())
    {
        DLOGE("Unable to open file:", "io", Severity::CRIT);
        DLOGI("file name= " + file_path.string(), "io", Severity::CRIT);

        return false;
    }

    stream = ifs;
    return true;
}

bool get_file_as_stream(const char* virtual_path, hash_t archive, std::istream& stream)
{
    auto it = archives_.find(archive);
    if(it != archives_.end())
    {
        DLOGE("Cannot find unknown archive:", "io", Severity::CRIT);
        DLOGI(std::to_string(key) + " -> " + HRESOLVE(key), "io", Severity::CRIT);
        return false;
    }

    zipios::FileCollection::stream_pointer_t in_stream(it->second.getInputStream(virtual_path));

    if(!in_stream.good())
    {
        DLOGE("Unable to form stream:", "io", Severity::CRIT);
        DLOGI("from archive: " + std::to_string(archive) + " -> " + HRESOLVE(archive), "io", Severity::CRIT);
        DLOGI("virtual path: " + std::string(virtual_path), "io", Severity::CRIT);

        return false;
    }

    stream = in_stream;
    return true;
}*/

} // namespace io
} // namespace wcore
