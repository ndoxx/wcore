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
    std::map<hash_t, zipios::ZipFile> archives;
    std::map<hash_t, std::map<hash_t, std::string>> vpaths;
    XMLParser xml_parser;
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
    if(pimpl_->archives.find(key) != pimpl_->archives.end())
    {
        DLOGW("Ignoring already loaded archive:", "io", Severity::CRIT);
        DLOGI("key:  " + std::to_string(key) + " -> " + HRESOLVE(key), "io", Severity::CRIT);
        return false;
    }

    if(!fs::exists(file_path))
    {
        DLOGE("Archive does not exist:", "io", Severity::CRIT);
        DLOGI("path: <p>" + file_path.string() + "</p>", "io", Severity::CRIT);
        DLOGI("key:  " + std::to_string(key) + " -> " + HRESOLVE(key), "io", Severity::CRIT);
        return false;
    }
    pimpl_->archives.insert(std::pair(key, zipios::ZipFile(file_path.string().c_str())));

    // * Parse manifest inside archive
    DLOGI("Reading manifest.", "io", Severity::LOW);
    auto stream = get_file_as_stream("MANIFEST.xml", key);
    pimpl_->xml_parser.load_file_xml(*stream);
    pimpl_->vpaths[key] = {};

    // Each vpath (virtual path) node holds similar information to folder nodes in config
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

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const fs::path& file_path)
{
    std::shared_ptr<std::ifstream> ifs = std::make_shared<std::ifstream>(std::ifstream(file_path));

    if(!ifs->is_open())
    {
        DLOGE("Unable to open file:", "io", Severity::CRIT);
        DLOGI("file name= " + file_path.string(), "io", Severity::CRIT);

        return nullptr;
    }

    return ifs;
}

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const char* virtual_path, hash_t archive)
{
    auto it = pimpl_->archives.find(archive);
    if(it == pimpl_->archives.end())
    {
        DLOGE("Cannot find unknown archive:", "io", Severity::CRIT);
        DLOGI(std::to_string(archive) + " -> " + HRESOLVE(archive), "io", Severity::CRIT);
        return nullptr;
    }

    zipios::FileCollection::stream_pointer_t in_stream(it->second.getInputStream(virtual_path));

    if(!in_stream->good())
    {
        DLOGE("Unable to form stream:", "io", Severity::CRIT);
        DLOGI("from archive: " + std::to_string(archive) + " -> " + HRESOLVE(archive), "io", Severity::CRIT);
        DLOGI("virtual path: " + std::string(virtual_path), "io", Severity::CRIT);

        return nullptr;
    }

    return in_stream;
}

std::shared_ptr<std::istream> FileSystem::get_file_as_stream(const char* filename,
                                                             hash_t folder_node,
                                                             hash_t archive)
{
    // * First, try to get stream from archive
    const auto& vpaths = pimpl_->vpaths.at(archive);
    auto itvpath = vpaths.find(folder_node);
    if(itvpath != vpaths.end())
    {
        const std::string& vpath = itvpath->second;
        return get_file_as_stream((vpath+filename).c_str(), archive);
    }

    // * Couldn't find file in archive, try in folders
    fs::path file_path;
    if(CONFIG.get(folder_node, file_path))
        return get_file_as_stream(file_path / filename);

    return nullptr;
}


} // namespace wcore
