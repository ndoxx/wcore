#include "informer.h"

namespace wcore
{

size_t Informer::N_INST = 0;
const size_t Informer::MAX_DELEGATES = 1024;

Informer::Informer()
:subscriber_map_(), delegate_ids_(), id_(++N_INST)
{
    //ctor
}

Informer::~Informer()
{
    //dtor
}

WDelegateID Informer::add_delegate(hash_t chan, WpFunc delegate)
{
    assert(subscriber_map_.size()<MAX_DELEGATES && "Can't add more delegates.");
    auto it = subscriber_map_.insert(std::make_pair(chan,delegate));
    // Make a unique delegate ID
    WDelegateID del_id = (WDelegateID) (MAX_DELEGATES*id_+subscriber_map_.size());
    delegate_ids_.insert(std::make_pair(del_id,it));
    return del_id;
}

void Informer::remove_delegate(WDelegateID del_id)
{
    subscriber_map_.erase(delegate_ids_.at(del_id));
}

}
