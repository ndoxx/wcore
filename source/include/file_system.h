#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <filesystem>
#include <string>

#include "wtypes.h"
#include "singleton.hpp"

namespace fs = std::filesystem;

namespace wcore
{

class FileSystem: public Singleton<FileSystem>
{
private:
    FileSystem(const FileSystem&){};
    FileSystem();
    virtual ~FileSystem();

public:
    friend FileSystem& Singleton<FileSystem>::Instance();
    friend void Singleton<FileSystem>::Kill();

    // Open archive, associate it to a hash key
    bool open_archive(const fs::path& file_path, hash_t key);
    // Close archive
    bool close_archive(hash_t key);
    // Get file as stream specifying a physical file path
    std::shared_ptr<std::istream> get_file_as_stream(const fs::path& file_path);
    // Get file as stream from archive
    std::shared_ptr<std::istream> get_file_as_stream(const char* virtual_path, hash_t archive);
    // Get file as stream, try from folder first then archive
    std::shared_ptr<std::istream> get_file_as_stream(const char* filename,
                                                     hash_t folder_node,
                                                     hash_t archive);
    // Get file as string, try from folder first then archive
    std::string get_file_as_string(const char* filename,
                                   hash_t folder_node,
                                   hash_t archive);

private:
    struct Impl;
    std::shared_ptr<Impl> pimpl_; // opaque pointer
};

#define FILESYSTEM FileSystem::Instance()

} // namespace wcore

#endif // FILE_SYSTEM_H
