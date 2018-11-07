#include "wcomponent.h"

WComponent::~WComponent()
{
    //dtor
}


void component::destroy(const WComponent* comp)
{
    delete comp;
}
