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
    DLOGN("[FileSystem] Opening archive:", "io", Severity::LOW);
    DLOGI("path: <p>" + file_path.string() + "</p>", "io", Severity::CRIT);
    DLOGI("key:  " + std::to_string(key) + " -> <n>" + HRESOLVE(key) + "</n>", "io", Severity::CRIT);

    // Locate archive
    if(pimpl_->archives.find(key) != pimpl_->archives.end())
    {
        DLOGW("Ignoring already loaded archive.", "io", Severity::CRIT);
        return false;
    }

    // Sanity check
    if(!fs::exists(file_path))
    {
        DLOGE("Archive does not exist.", "io", Severity::CRIT);
        return false;
    }

    // Open and register archive
    pimpl_->archives.insert(std::pair(key, zipios::ZipFile(file_path.string().c_str())));

    // * Parse manifest inside archive
    DLOGI("<i>Reading manifest.</i>", "io", Severity::LOW);
    auto stream = get_file_as_stream("MANIFEST.xml", key);

    // Check that a manifest is indeed present
    if(stream == nullptr)
    {
        DLOGE("Could not find MANIFEST.xml in archive.", "io", Severity::CRIT);
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
        DLOGI("<h>vpath</h>: " + vpath_name + " -> <p>" + vpath_value + "</p>", "io", Severity::LOW);
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
        DLOGE("Cannot close unknown archive:", "io", Severity::CRIT);
        DLOGI(std::to_string(key) + " -> <n>" + HRESOLVE(key) + "</n>", "io", Severity::CRIT);
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
    std::shared_ptr<std::ifstream> ifs = std::make_shared<std::ifstream>(std::ifstream(file_path));

    // Sanity check
    if(!ifs->is_open())
    {
        DLOGE("Unable to open file:", "io", Severity::CRIT);
        DLOGI("<p>" + file_path.string() + "</p>", "io", Severity::CRIT);

        return nullptr;
    }

    DLOGN("[FileSystem] Getting stream from file path:", "io", Severity::LOW);
    DLOGI("<p>" + file_path.string() + "</p>", "io", Severity::LOW);

    return ifs;
}

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const char* virtual_path, hash_t archive)
{
    // Locate archive
    auto it = pimpl_->archives.find(archive);
    if(it == pimpl_->archives.end())
    {
        DLOGE("Cannot find unknown archive:", "io", Severity::CRIT);
        DLOGI(std::to_string(archive) + " -> <n>" + HRESOLVE(archive) + "</n>", "io", Severity::CRIT);
        return nullptr;
    }

    // Get stream to archive file using zipios API
    zipios::FileCollection::stream_pointer_t in_stream(it->second.getInputStream(virtual_path));

    // Sanity check
    if(!in_stream->good())
    {
        DLOGE("Unable to form stream:", "io", Severity::CRIT);
        DLOGI("from archive: " + std::to_string(archive) + " -> <n>" + HRESOLVE(archive) + "</n>", "io", Severity::CRIT);
        DLOGI("virtual path: <p>" + std::string(virtual_path) + "</p>", "io", Severity::CRIT);

        return nullptr;
    }

    DLOGN("[FileSystem] Getting stream from archive:", "io", Severity::LOW);
    DLOGI(std::string("archive: ") + std::to_string(archive) + " -> <n>" + HRESOLVE(archive) + "</n>", "io", Severity::LOW);
    DLOGI(std::string("vpath:   <p>") + virtual_path + "</p>", "io", Severity::LOW);

    return in_stream;
}

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const char* filename,
                                                             hash_t folder_node,
                                                             hash_t archive)
{
    // * First, try to get stream from archive
    // Find virtual path to asset in archive (saved from the manifest)
    const auto& vpaths = pimpl_->vpaths.at(archive);
    auto itvpath = vpaths.find(folder_node);
    if(itvpath != vpaths.end())
    {
        // Found. Dispatch to archive stream creator
        const std::string& vpath = itvpath->second;
        auto stream = get_file_as_stream((vpath+filename).c_str(), archive);
        if(stream) return stream;
    }

    // * Couldn't find file in archive, try in folders
    // Get path from config folder node, dispatch to file stream creator
    fs::path file_path;
    if(CONFIG.get(folder_node, file_path))
        return get_file_as_stream(file_path / filename);

    return nullptr;
}


} // namespace wcore
