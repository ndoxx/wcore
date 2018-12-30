#include "intern_string.h"
#include "config.h"
#include "logger.h"

namespace wcore
{

void InternStringLocator::init()
{
    DLOGS("[InternStringLocator] Retrieving intern string table.", "core", Severity::LOW);
    fs::path xmlpath = CONFIG.get_config_directory() / "dbg_intern_strings.xml";
    if(!fs::exists(xmlpath))
    {
        DLOGE("[InternStringLocator] Cannot find intern string hash table file.", "core", Severity::WARN);
        DLOGI(xmlpath.string(), "core", Severity::WARN);
        DLOGI("Run the \"internstr\" utility.", "core", Severity::WARN);
        return;
    }
    xml_parser_.load_file_xml(xmlpath);
    retrieve_table(xml_parser_.get_root());
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

} // namespace wcore
