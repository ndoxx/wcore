#include <map>

#include "file_system.h"
#include "xml_parser.h"
#include "logger.h"
#include "config.h"

#include "vendor/zipios/zipfile.hpp"

namespace wcore
{

struct FileSystem::Impl
{
    std::map<hash_t, zipios::ZipFile> archives; // Open archives
    std::map<hash_t, std::map<hash_t, std::string>> vpaths; // Virtual paths inside loaded archives
    XMLParser xml_parser; // To parse the manifests inside archives
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
    DLOGN("[FileSystem] Opening archive:", "ios");
    DLOGI("path: <p>" + file_path.string() + "</p>", "ios");
    DLOGI("key:  " + std::to_string(key) + " -> <n>" + HRESOLVE(key) + "</n>", "ios");

    // Locate archive
    if(pimpl_->archives.find(key) != pimpl_->archives.end())
    {
        DLOGW("Ignoring already loaded archive.", "ios");
        return false;
    }

    // Sanity check
    if(!fs::exists(file_path))
    {
        DLOGE("Archive does not exist.", "ios");
        return false;
    }

    // Open and register archive
    pimpl_->archives.insert(std::pair(key, zipios::ZipFile(file_path.string().c_str())));

    // * Parse manifest inside archive
    DLOGI("<i>Reading manifest.</i>", "ios");
    auto stream = get_file_as_stream("MANIFEST.xml", key);

    // Check that a manifest is indeed present
    if(stream == nullptr)
    {
        DLOGE("Could not find MANIFEST.xml in archive.", "ios");
        return false;
    }

    // Parse manifest, initialize virtual paths map
    pimpl_->xml_parser.load_file_xml(*stream);
    pimpl_->vpaths[key] = {};

    // Each vpath (virtual path) node holds similar information to folder nodes in config.
    // vpaths are in fact folder node overrides in that regard
    rapidxml::xml_node<>* root = pimpl_->xml_parser.get_root();
    for(rapidxml::xml_node<>* cur_node=root->first_node("vpath");
        cur_node;
        cur_node=cur_node->next_sibling("vpath"))
    {
        std::string vpath_name, vpath_value;
        if(!xml::parse_attribute(cur_node, "name", vpath_name)) continue;
        if(!xml::parse_attribute(cur_node, "value", vpath_value)) continue;
        DLOGI("<h>vpath</h>: " + vpath_name + " -> <p>" + vpath_value + "</p>", "ios");
        pimpl_->vpaths[key][H_(vpath_name.c_str())] = vpath_value;
    }

    return true;
}

bool FileSystem::close_archive(hash_t key)
{
    // Locate archive
    auto it = pimpl_->archives.find(key);
    if(it == pimpl_->archives.end())
    {
        DLOGE("Cannot close unknown archive:", "ios");
        DLOGI(std::to_string(key) + " -> <n>" + HRESOLVE(key) + "</n>", "ios");
        return false;
    }

    // Close pack and remove entry
    it->second.close();
    pimpl_->archives.erase(it);
    return true;
}

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const fs::path& file_path)
{
    // Get stream to file
    std::shared_ptr<std::ifstream> ifs = std::make_shared<std::ifstream>(file_path);

    // Sanity check
    if(!ifs->is_open())
    {
        DLOGE("Unable to open file:", "ios");
        DLOGI("<p>" + file_path.string() + "</p>", "ios");

        return nullptr;
    }

    DLOGN("[FileSystem] Getting stream from file path:", "ios");
    DLOGI("<p>" + file_path.string() + "</p>", "ios");

    return ifs;
}

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const char* virtual_path, hash_t archive)
{
    // Locate archive
    auto it = pimpl_->archives.find(archive);
    if(it == pimpl_->archives.end())
    {
        DLOGE("Cannot find unknown archive:", "ios");
        DLOGI(std::to_string(archive) + " -> <n>" + HRESOLVE(archive) + "</n>", "ios");
        return nullptr;
    }

    // Check if entry exists in archive
    zipios::FileEntry::pointer_t entry(it->second.getEntry(virtual_path));
    if(entry)
    {
        // Get stream to archive file using zipios API
        zipios::FileCollection::stream_pointer_t in_stream(it->second.getInputStream(virtual_path));

        // Sanity check
        if(!in_stream->good())
        {
            DLOGE("Unable to form stream:", "ios");
            DLOGI("from archive: " + std::to_string(archive) + " -> <n>" + HRESOLVE(archive) + "</n>", "ios");
            DLOGI("virtual path: <p>" + std::string(virtual_path) + "</p>", "ios");

            return nullptr;
        }

        DLOGN("[FileSystem] Getting stream from archive:", "ios");
        DLOGI(std::string("archive: ") + std::to_string(archive) + " -> <n>" + HRESOLVE(archive) + "</n>", "ios");
        DLOGI(std::string("<h>vpath</h>:   <p>") + virtual_path + "</p>", "ios");

        return in_stream;
    }

    return nullptr;
}

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const char* filename,
                                                             hash_t folder_node,
                                                             hash_t archive)
{
    // * First, try to find file in folders
    // Get path from config folder node, dispatch to file stream creator
    fs::path file_path;
    if(CONFIG.get(folder_node, file_path))
    {
        file_path /= filename;
        if(fs::exists(file_path))
        {
            auto stream = get_file_as_stream(file_path);
            if(stream)
                return stream;
        }
    }

    // * Couldn't find file in folders, try to get stream from archive
    // Find virtual path to asset in archive (saved from the manifest)
    auto itvpaths = pimpl_->vpaths.find(archive);
    if(itvpaths != pimpl_->vpaths.end())
    {
        const auto& vpaths = itvpaths->second;
        auto itvpath = vpaths.find(folder_node);
        if(itvpath != vpaths.end())
        {
            // Found. Dispatch to archive stream creator
            const std::string& vpath = itvpath->second;
            auto stream = get_file_as_stream((vpath+filename).c_str(), archive);
            if(stream)
                return stream;
        }
    }

    DLOGE("[FileSystem] File couldn't be reached:", "ios");
    DLOGI("filename: <p>" + std::string(filename) + "</p>", "ios");
    DLOGI("folder node: " + std::to_string(folder_node) + " -> <x>" + HRESOLVE(folder_node) + "</x>", "ios");
    DLOGI("archive:     " + std::to_string(archive) + " -> <h>" + HRESOLVE(archive) + "</h>", "ios");

    return nullptr;
}

std::string FileSystem::get_file_as_string(const char* filename,
                                           hash_t folder_node,
                                           hash_t archive)
{
    auto pstream = get_file_as_stream(filename, folder_node, archive);
    if(!pstream)
        return "";

    // Read file to buffer
    return std::string((std::istreambuf_iterator<char>(*pstream)),
                        std::istreambuf_iterator<char>());
}

bool FileSystem::file_exists(const char* filename,
                             hash_t folder_node,
                             hash_t archive)
{
    if(strlen(filename)==0)
        return false;

    fs::path file_path;
    if(CONFIG.get(folder_node, file_path))
    {
        file_path /= filename;
        if(fs::exists(file_path))
            return true;
    }

    auto itvpaths = pimpl_->vpaths.find(archive);
    if(itvpaths != pimpl_->vpaths.end())
    {
        const auto& vpaths = itvpaths->second;
        auto itvpath = vpaths.find(folder_node);
        if(itvpath != vpaths.end())
        {
            auto itarch = pimpl_->archives.find(archive);
            if(itarch == pimpl_->archives.end())
            {
                const std::string& vpath = itvpath->second;
                zipios::FileEntry::pointer_t entry(itarch->second.getEntry((vpath+filename).c_str()));
                if(entry)
                    return true;
            }
        }
    }

    return false;
}

} // namespace wcore
