#include <fstream>
#include "vendor/rapidxml/rapidxml_print.hpp"

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

XMLParser::XMLParser(std::istream& stream)
{
    load_file_xml(stream);
}

XMLParser::~XMLParser()
{

}

// deprec
void XMLParser::load_file_xml(const fs::path& filepath)
{
    if(!fs::exists(filepath))
    {
        DLOGE("Wrong path: ", "core");
        DLOGI(filepath.string(), "core");
        return;
    }
    filepath_ = filepath;

    DLOGN("[XML] Parsing xml file:", "core");
    DLOGI("<p>" + filepath.string() + "</p>", "core");

    // Read the xml file into a vector
    buffer_ = io::get_file_as_vector(filepath);

    // Parse the buffer using the xml file parsing library into DOM
    dom_.parse<0>(&buffer_[0]);

    // Find our root node
    root_ = dom_.first_node();
    if(!root_)
    {
        DLOGE("[XML] No root node.", "core");
        return;
    }
}

void XMLParser::load_file_xml(std::istream& stream)
{
    // Sanity check
    if(!stream.good())
    {
        DLOGE("[XML] Input stream error.", "core");
        return;
    }

    DLOGN("[XML] Parsing xml file from stream.", "core");

    // Read the xml file into a vector
    buffer_ = std::vector<char>((std::istreambuf_iterator<char>(stream)),
                                 std::istreambuf_iterator<char>());
    buffer_.push_back('\0');

    // Parse the buffer using the xml file parsing library into DOM
    dom_.parse<0>(&buffer_[0]);

    // Find our root node
    root_ = dom_.first_node();
    if(!root_)
        DLOGE("[XML] No root node.", "core");
}

// deprec
void XMLParser::write()
{
    std::ofstream file(filepath_);
    file << dom_;
    file.close();
}

void XMLParser::write(std::ostream& stream)
{
    stream << dom_;
}

void XMLParser::reset()
{
    dom_.clear();
    buffer_.clear();
}

char* XMLParser::allocate_string(const char* str)
{
    return dom_.allocate_string(str);
}


#ifdef __DEBUG__
void XMLParser::print_document()
{
    std::cout << dom_ << std::endl;
}
#endif

}
