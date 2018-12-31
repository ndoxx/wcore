#ifndef INTERN_STRING_H
#define INTERN_STRING_H

#include <string>
#include <map>

#include "wtypes.h"
#include "xml_parser.h"
#include "singleton.hpp"

namespace wcore
{

class InternStringLocator : public Singleton<InternStringLocator>
{
public:
    friend InternStringLocator& Singleton<InternStringLocator>::Instance();
    friend void Singleton<InternStringLocator>::Kill();

    std::string operator()(hash_t hashname);
    void init();
    void add_intern_string(const std::string& str);

private:
    InternStringLocator (const InternStringLocator&){};
    InternStringLocator();
   ~InternStringLocator();

    void retrieve_table(rapidxml::xml_node<>* node);

    XMLParser xml_parser_;
    std::map<hash_t, std::string> intern_strings_;
};


#define HRESOLVE InternStringLocator::Instance()

} // namespace wcore


#endif // INTERN_STRING_H
