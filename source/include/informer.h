#ifndef INFORMER_H
#define INFORMER_H

#include <map>
#include <type_traits>
#include <cassert>

#include "message.h"

class Informer
{
    friend class Listener;

    protected:
        typedef std::multimap<hash_t,WpFunc> DelegateList;
        typedef std::map<WDelegateID,DelegateList::iterator> DelegateIDMap;
        typedef DelegateList::const_iterator   cIter;
        typedef std::pair<cIter,cIter>         Range;

    public:
        Informer();
        virtual ~Informer();

        inline const DelegateList& get_delegates() const;
        inline WID get_WID() const;

    protected:
        // Multicast delegation. Perfect forward event data, checks that data inherits from struct WData
        template <typename T,
                  typename = typename std::enable_if<std::is_base_of<WData,T>::value >::type>
        void post(hash_t, T&&);

    private:
        WDelegateID add_delegate(hash_t, WpFunc delegate);
        void remove_delegate(WDelegateID);
        static size_t N_INST;
        static const size_t MAX_DELEGATES;

    protected:
        DelegateList   delegates_;
        DelegateIDMap  delegate_ids_;
        WID id_;
};

inline const Informer::DelegateList& Informer::get_delegates() const
{
    return delegates_;
}

inline WID Informer::get_WID() const
{
    return id_;
}

template <typename T, typename>
void Informer::post(hash_t message_type, T&& data)
{
    data.sender_ = id_;
    Range range = delegates_.equal_range(message_type);
    for(cIter it=range.first; it!=range.second; ++it)
        (it->second)(std::forward<T>(data));
}

#endif // INFORMER_H
