#ifndef INTERN_STRING_H
#define INTERN_STRING_H

#include <string>
#include <map>

#include "wtypes.h"
#include "xml_parser.h"

namespace wcore
{

class InternStringLocator
{
public:
    std::string operator()(hash_t hashname);
    void init();

private:
    void retrieve_table(rapidxml::xml_node<>* node);

    XMLParser xml_parser_;
    std::map<hash_t, std::string> intern_strings_;
};


static InternStringLocator HRESOLVE;

} // namespace wcore


#endif // INTERN_STRING_H
