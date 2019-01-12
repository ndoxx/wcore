#include "informer.h"

namespace wcore
{

size_t Informer::N_INST = 0;

Informer::Informer()
:subscriber_map_(),
delegate_ids_(),
id_(++N_INST)
{
    //ctor
}

Informer::~Informer()
{
    //dtor
}

WDelegateID Informer::add_delegate(hash_t chan, WpFunc delegate)
{
    subscriber_map_[chan].push_back(delegate);
    // Make a unique delegate ID
    WDelegateID del_id = chan;
    del_id ^= subscriber_map_[chan].size() + 0x9e3779b9 + (del_id<<6) + (del_id>>2);
    auto it = subscriber_map_[chan].end();
    --it;
    delegate_ids_.insert(std::make_pair(del_id,it));
    return del_id;
}

void Informer::remove_delegate(hash_t chan, WDelegateID del_id)
{
    subscriber_map_.at(chan).erase(delegate_ids_.at(del_id));
}

}
