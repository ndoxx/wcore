#include "component.h"

namespace wcore
{

WComponent::~WComponent()
{
    //dtor
}


void component::destroy(const WComponent* comp)
{
    delete comp;
}

}
