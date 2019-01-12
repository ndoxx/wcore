#ifndef INFORMER_H
#define INFORMER_H

#include <map>
#include <list>
#include <type_traits>
#include <cassert>
#include <iostream>

#include "message.h"

namespace wcore
{

class Informer
{
    friend class Listener;

    protected:
        typedef std::list<WpFunc> DelegateList;
        typedef std::map<hash_t,DelegateList> SubscriberMap;
        typedef std::map<WDelegateID,DelegateList::iterator> DelegateIDMap;

    public:
        Informer();
        virtual ~Informer();

        inline const SubscriberMap& get_delegates() const;
        inline WID get_WID() const;

    protected:
        // Multicast delegation. Perfect forward event data, checks that data inherits from struct WData
        template <typename T,
                  typename = typename std::enable_if<std::is_base_of<WData,T>::value >::type>
        void post(hash_t, T&&);

    private:
        WDelegateID add_delegate(hash_t, WpFunc delegate);
        void remove_delegate(hash_t chan, WDelegateID);
        static size_t N_INST;
        static const size_t MAX_DELEGATES;

    protected:
        SubscriberMap  subscriber_map_;
        DelegateIDMap  delegate_ids_;
        WID id_;
};

inline const Informer::SubscriberMap& Informer::get_delegates() const
{
    return subscriber_map_;
}

inline WID Informer::get_WID() const
{
    return id_;
}

template <typename T, typename>
void Informer::post(hash_t message_type, T&& data)
{
    data.sender_ = id_;
    const DelegateList& dlist = subscriber_map_.at(message_type);
    for(auto&& delegate: dlist)
    {
        // Execute delegate and break if event was consumed
        if(!delegate(std::forward<T>(data)))
            break;
    }
}

}

#endif // INFORMER_H
