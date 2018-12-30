#include <fstream>

#include "io_utils.h"
#include "config.h"
#include "logger.h"

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
#ifdef __DEBUG__
        DLOGE("Unable to open file:", "io", Severity::CRIT);
        DLOGI("node= " + std::to_string(folder_node), "io", Severity::CRIT);
        DLOGI("file name= " + file_name.string(), "io", Severity::CRIT);
        DLOGI("void string returned.", "io", Severity::CRIT);
#endif
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
#ifdef __DEBUG__
        DLOGE("Unable to open file:", "io", Severity::CRIT);
        DLOGI("file name= " + file_path.string(), "io", Severity::CRIT);
        DLOGI("void vector returned.", "io", Severity::CRIT);
#endif
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
#ifdef __DEBUG__
        DLOGE("Unable to open binary file:", "io", Severity::CRIT);
        DLOGI("file name= " + file_path.string(), "io", Severity::CRIT);
        DLOGI("void vector returned.", "io", Severity::CRIT);
#endif
        return std::vector<char>();
    }

    std::vector<char> result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    return result;
}


} // namespace io
} // namespace wcore
