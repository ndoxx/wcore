#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <filesystem>
#include <string>

#include "utils.h"

namespace wcore
{
namespace fs = std::filesystem;
namespace io
{

// Return path to a file given folder config node and file name. Void path if error.
extern fs::path get_file(hashstr_t folder_node, const char* file_name);
extern inline fs::path get_file(hashstr_t folder_node, const std::string& file_name)
{
    return get_file(folder_node, file_name.c_str());
}

} // namespace io
} // namespace wcore
#endif // IO_UTILS_H
