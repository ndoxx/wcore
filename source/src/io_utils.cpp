#include "io_utils.h"
#include "config.h"
#include "logger.h"

namespace wcore
{
namespace io
{

fs::path get_file(hashstr_t folder_node, const char* file_name)
{
    fs::path file_path;
    if(!CONFIG.get(folder_node, file_path))
    {
        DLOGE("Missing folder config node: <n>" + std::to_string(folder_node) + "</n>", "core", Severity::CRIT);
        return fs::path();
    }

    file_path /= file_name;
    if(!fs::exists(file_path))
    {
        DLOGE("File not found: <p>" + std::string(file_name) + "</p>", "core", Severity::CRIT);
        return fs::path();
    }

    return file_path;
}

} // namespace io
} // namespace wcore
