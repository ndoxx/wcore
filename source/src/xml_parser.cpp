#include "xml_parser.h"
#include "logger.h"
#include "io_utils.h"

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

void XMLParser::load_file_xml(const fs::path& filepath)
{
    if(!fs::exists(filepath))
    {
        DLOGE("Wrong path: ", "core", Severity::CRIT);
        DLOGI(filepath.string(), "core", Severity::CRIT);
        return;
    }

#ifdef __DEBUG__
    DLOGN("[XML] Parsing xml file:", "core", Severity::LOW);
    DLOGI("<p>" + filepath.string() + "</p>", "core", Severity::LOW);
#endif

    // Read the xml file into a vector
    buffer_ = io::get_file_as_vector(filepath);

    // Parse the buffer using the xml file parsing library into DOM
    dom_.parse<0>(&buffer_[0]);

    // Find our root node
    root_ = dom_.first_node();
    if(!root_)
    {
#ifdef __DEBUG__
        DLOGE("[XML] No root node.", "core", Severity::CRIT);
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
