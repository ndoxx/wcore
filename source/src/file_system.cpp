#include <map>

#include "file_system.h"
#include "logger.h"

#include "vendor/zipios/zipfile.hpp"

namespace wcore
{

struct FileSystem::Impl
{
    std::map<hash_t, zipios::ZipFile> archives;
};

FileSystem::FileSystem():
pimpl_(new Impl)
{

}

// dtor needed for unique_ptr pimpl to work
FileSystem::~FileSystem()
{
    for(auto&& [key, zipfile]: pimpl_->archives)
        zipfile.close();
}


bool FileSystem::open_archive(const fs::path& file_path, hash_t key)
{
    if(!fs::exists(file_path))
    {
        DLOGE("Archive does not exist:", "io", Severity::CRIT);
        DLOGI("path: <p>" + file_path.string() + "</p>", "io", Severity::CRIT);
        DLOGI("key:  " + std::to_string(key) + " -> " + HRESOLVE(key), "io", Severity::CRIT);
        return false;
    }
    pimpl_->archives.insert(std::pair(key, zipios::ZipFile(file_path.string().c_str())));
    return true;
}

bool FileSystem::close_archive(hash_t key)
{
    auto it = pimpl_->archives.find(key);
    if(it == pimpl_->archives.end())
    {
        DLOGE("Cannot close unknown archive:", "io", Severity::CRIT);
        DLOGI(std::to_string(key) + " -> " + HRESOLVE(key), "io", Severity::CRIT);
        return false;
    }

    it->second.close();
    pimpl_->archives.erase(it);
    return true;
}

} // namespace wcore
