#include "listener.h"
#include "informer.h"

namespace wcore
{

Listener::Listener()
:delegate_ids_()
{
    //ctor
}

Listener::~Listener()
{
    //dtor
}

void Listener::subscribe(hash_t chan, Informer& informer, WpFunc delegate)
{
    // Compute key and look for previous equivalent subscription
    WID  inf_id = informer.get_WID();
    auto key = std::make_pair(chan, inf_id);
    if(delegate_ids_.count(key)>0) return;

    // Ask informer to register delegate and associate it to channel event.
    // Retrieve delegate id.
    WDelegateID del_id = informer.add_delegate(chan, delegate);

    // Save delegate ID and informer ID for later use.
    delegate_ids_.insert(std::make_pair(key,del_id));
}

void Listener::unsubscribe(hash_t chan, Informer& informer)
{
    // Find delegate ID associated to message type and ask informer to drop it.
    auto key = std::make_pair(chan, informer.get_WID());
    informer.remove_delegate(chan, delegate_ids_.at(key));
    // Remove reference in the delegate ID map.
    delegate_ids_.erase(key);
}

}
