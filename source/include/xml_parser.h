#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <filesystem>
#include <istream>
#include <ostream>

#include "vendor/rapidxml/rapidxml.hpp"
#include "xml_utils.hpp"

namespace wcore
{

namespace fs = std::filesystem;

class XMLParser
{
private:
    rapidxml::xml_document<> dom_;
    rapidxml::xml_node<>* root_;
    std::vector<char> buffer_; // Rapidxml is an in-situ parser -> we need to save text data
    fs::path filepath_;

public:
    XMLParser();

    [[deprecated("use streams instead")]]
    XMLParser(const char* filename);

    XMLParser(std::istream& stream);
    ~XMLParser();

    [[deprecated("use streams instead")]]
    void load_file_xml(const fs::path& filepath);

    void load_file_xml(std::istream& stream);
    void reset();
    //[[deprecated("use streams instead")]]
    void write();
    void write(std::ostream& stream);

    char* allocate_string(const char* str);

    inline rapidxml::xml_node<>* get_root()         { return root_; }
    inline rapidxml::xml_document<>& get_document() { return dom_; }

#ifdef __DEBUG__
    void print_document();
#endif
};

}
#endif // XML_PARSER_H
