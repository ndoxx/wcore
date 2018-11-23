#include "informer.h"

namespace wcore
{

size_t Informer::N_INST = 0;
const size_t Informer::MAX_DELEGATES = 1024;

Informer::Informer()
:delegates_(), delegate_ids_(), id_(++N_INST)
{
    //ctor
}

Informer::~Informer()
{
    //dtor
}

WDelegateID Informer::add_delegate(hash_t chan, WpFunc delegate)
{
    assert(delegates_.size()<MAX_DELEGATES && "Can't add more delegates.");
    auto it = delegates_.insert(std::make_pair(chan,delegate));
    // Make a unique delegate ID
    WDelegateID del_id = (WDelegateID) (MAX_DELEGATES*id_+delegates_.size());
    delegate_ids_.insert(std::make_pair(del_id,it));
    return del_id;
}

void Informer::remove_delegate(WDelegateID del_id)
{
    delegates_.erase(delegate_ids_.at(del_id));
}

}
