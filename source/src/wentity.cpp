#include "wentity.h"
#include "logger.h"

namespace wcore
{

WEntity::WEntity()
: components_()
{
    //ctor
}

WEntity::~WEntity()
{
    for(auto it: components_)
    {
        delete it.second;
    }
}

#ifdef __DEBUG__
void WEntity::warn_duplicate_component(const char* name)
{
    std::stringstream ss;
    ss << "Ignoring duplicate component: " << name;
    DLOGW(ss.str(), "entity", Severity::WARN);
}
#endif

}
