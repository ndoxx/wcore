#include <sstream>

#include "message_tracker.h"
#include "logger.h"

namespace wcore
{

void MessageTracker::track(hash_t channel, Informer& informer)
{
    subscribe(channel, informer, std::bind(display, channel, std::placeholders::_1));
}

bool MessageTracker::display(hash_t channel, const WData& wdata)
{
#ifdef __DEBUG__
    std::string dataStr(wdata.to_string());
    std::stringstream ss;
    ss << channel << " -> " << ((dataStr.size()>0)?dataStr:"[NODATA]");
    DLOGT(ss.str(), "default", Severity::LOW);
#endif
    return true; // Do NOT consume event
}

}
