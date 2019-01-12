#ifndef LISTENER_H
#define LISTENER_H

#include <map>
#include <functional>
#include <utility>
#include "message.h"

namespace wcore
{

class Informer;
class Listener
{
    protected:
        typedef std::pair<hash_t,WID>    LinkKey;
        typedef std::map<LinkKey,WDelegateID>     DelegateIDMap;

    public:
        Listener();
        virtual ~Listener();

        virtual void subscribe(hash_t, Informer&, WpFunc);
        virtual void unsubscribe(hash_t, Informer&);

        template <typename T>
        void subscribe(hash_t, Informer&, bool (T::*func)(const WData&));

    protected:
        DelegateIDMap delegate_ids_;
};

template <typename T>
void Listener::subscribe(hash_t chan, Informer& informer, bool (T::*func)(const WData&))
{
    WpFunc delegate = dlg::make_delegate(func, *this);
    subscribe(chan, informer, delegate);
}

}

#endif // LISTENER_H
