#ifndef XML_PARSER_H
#define XML_PARSER_H

#include "rapidxml/rapidxml.hpp"
#include "xml_utils.hpp"

class XMLParser
{
private:
    rapidxml::xml_document<> dom_;
    rapidxml::xml_node<>* root_;
    std::vector<char> buffer_; // Rapidxml is an in-situ parser -> we need to save text data

public:
    XMLParser();
    XMLParser(const char* filename);
    ~XMLParser();

    void load_file_xml(const char* filename);
    void reset();

    inline rapidxml::xml_node<>* get_root() { return root_; }
};

#endif // XML_PARSER_H
