#include "xml_parser.h"
#include "logger.h"

namespace wcore
{

XMLParser::XMLParser() = default;

XMLParser::XMLParser(const char* filename)
{
    load_file_xml(filename);
}

XMLParser::~XMLParser()
{

}

void XMLParser::load_file_xml(const char* filename)
{
#ifdef __DEBUG__
    DLOGN("[XML] Parsing xml file:", "default", Severity::LOW);
    DLOGI("<p>" + std::string(filename) + "</p>", "default", Severity::LOW);
#endif

    // Read the xml file into a vector
    std::ifstream xfile(filename);
    buffer_ = std::vector<char>((std::istreambuf_iterator<char>(xfile)), std::istreambuf_iterator<char>());
    buffer_.push_back('\0');

    // Parse the buffer using the xml file parsing library into DOM
    dom_.parse<0>(&buffer_[0]);

    // Find our root node
    root_ = dom_.first_node();
    if(!root_)
    {
#ifdef __DEBUG__
        DLOGE("[XML] No root node.", "default", Severity::CRIT);
#endif
        return;
    }
}

void XMLParser::reset()
{
    dom_.clear();
    buffer_.clear();
}

}
