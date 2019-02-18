#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <filesystem>
#include <string>
#include <vector>
#include <istream>
#include <ostream>

#include "wtypes.h"

namespace wcore
{
namespace fs = std::filesystem;
namespace io
{

// Return path to a file given folder config node and file name. Void path if error.
extern fs::path get_file(hash_t folder_node, const fs::path& file_name);
inline fs::path get_file(hash_t folder_node, const char* file_name)
{
    return get_file(folder_node, fs::path(file_name));
}
inline fs::path get_file(hash_t folder_node, const std::string& file_name)
{
    return get_file(folder_node, fs::path(file_name));
}

// Return a text string from a config node and file name.
extern std::string get_file_as_string(hash_t folder_node, const fs::path& file_name);
inline std::string get_file_as_string(hash_t folder_node, const char* file_name)
{
    return get_file_as_string(folder_node, fs::path(file_name));
}
inline std::string get_file_as_string(hash_t folder_node, const std::string& file_name)
{
    return get_file_as_string(folder_node, fs::path(file_name));
}

// Get the contents of a text file into a char vector (for rapidxml)
extern std::vector<char> get_file_as_vector(const fs::path& file_path);

// Get the contents of a binary file into a char vector
extern std::vector<char> get_binary_file_as_vector(const fs::path& file_path);


// ----------- WIP -----------

// Open archive, associate it to a hash key
extern void open_archive(const fs::path& file_path, hash_t key);
// Close archive
extern void close_archive(hash_t key);
/*
// Get a file as stream specifying a physical file path
extern bool get_file_as_stream(const fs::path& file_path, std::istream& stream);
// Get a file as stream from archive
extern bool get_file_as_stream(const char* virtual_path, hash_t archive, std::istream& stream);
*/

} // namespace io
} // namespace wcore
#endif // IO_UTILS_H
