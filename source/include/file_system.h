#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <filesystem>

#include "game_system.h"

namespace fs = std::filesystem;

namespace wcore
{

class FileSystem: public InitializerSystem
{
public:
    FileSystem();
    virtual ~FileSystem();

    // Open archive, associate it to a hash key
    bool open_archive(const fs::path& file_path, hash_t key);
    // Close archive
    bool close_archive(hash_t key);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_; // opaque pointer
};

} // namespace wcore

#endif // FILE_SYSTEM_H
