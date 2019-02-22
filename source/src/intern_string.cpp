#include "intern_string.h"
#include "config.h"
#include "logger.h"
#include "xml_parser.h"
#include "file_system.h"
#include "wtypes.h"

namespace wcore
{

InternStringLocator::InternStringLocator():
xml_parser_(new XMLParser())
{

}

InternStringLocator::~InternStringLocator()
{
    delete xml_parser_;
}

void InternStringLocator::init()
{
    DLOGS("[InternStringLocator] Retrieving intern string table.", "core", Severity::LOW);

    auto pstream = FILESYSTEM.get_file_as_stream("dbg_intern_strings.xml", "root.folders.config"_h, "pack0"_h);
    if(pstream == nullptr)
    {
        DLOGE("[InternStringLocator] Cannot find intern string hash table file.", "core", Severity::WARN);
        DLOGI("<p>config/dbg_intern_strings.xml</p>", "core", Severity::WARN);
        DLOGI("Run the \"internstr\" utility.", "core", Severity::WARN);
        return;
    }
    xml_parser_->load_file_xml(*pstream);

    retrieve_table(xml_parser_->get_root());
    DLOGES("core", Severity::LOW);
}

void InternStringLocator::retrieve_table(rapidxml::xml_node<>* node)
{
    for(rapidxml::xml_node<>* istr_node=node->first_node("string");
        istr_node;
        istr_node=istr_node->next_sibling("string"))
    {
        hash_t key;
        std::string value;
        if(!xml::parse_attribute(istr_node, "key", key)) continue;
        if(!xml::parse_attribute(istr_node, "value", value)) continue;
        intern_strings_.insert(std::make_pair(key,value));
    }
}

std::string InternStringLocator::operator()(hash_t hashname)
{
    auto it = intern_strings_.find(hashname);
    if(it!=intern_strings_.end())
        return it->second;
    else
        return std::string("???");
}

void InternStringLocator::add_intern_string(const std::string& str)
{
    hash_t hname = H_(str.c_str());
    if(intern_strings_.find(hname)==intern_strings_.end())
        intern_strings_.insert(std::make_pair(hname,str));
}


} // namespace wcore
