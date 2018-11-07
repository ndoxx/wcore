#include "wentity.h"

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
