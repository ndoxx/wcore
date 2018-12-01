#include "wentity.h"

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

}
