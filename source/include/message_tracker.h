#ifndef MESSAGE_TRACKER_H
#define MESSAGE_TRACKER_H

#include "listener.h"

namespace wcore
{

class MessageTracker: public Listener
{
public:
    MessageTracker() = default;
    virtual ~MessageTracker() = default;

    void track(hash_t channel, Informer& informer);

private:
    static void display(hash_t channel, const WData& wdata);
};

}

#endif // MESSAGE_TRACKER_H
