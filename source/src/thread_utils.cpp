#ifdef __linux__
    #include <pthread.h>
    #include <sched.h>
#elif _WIN32

#endif

#include <iostream>

#include "thread_utils.h"

namespace wcore
{
namespace thread
{

#ifdef __linux__
void max_thread_priority()
{
    /*
    pthread_t thId = pthread_self();
    pthread_attr_t thAttr;
    int policy = 0;
    int max_prio_for_policy = 0;

    pthread_attr_init(&thAttr);
    pthread_attr_getschedpolicy(&thAttr, &policy);
    max_prio_for_policy = sched_get_priority_max(policy);


    pthread_setschedprio(thId, max_prio_for_policy);
    pthread_attr_destroy(&thAttr);*/

    int ret;

    // We'll operate on the currently running thread.
    pthread_t this_thread = pthread_self();

    // struct sched_param is used to store the scheduling priority
    struct sched_param params;

    // We'll set the priority to the maximum.
    params.sched_priority = sched_get_priority_max(SCHED_FIFO);

    std::cout << "Trying to set thread realtime prio = " << params.sched_priority << std::endl;

    // Attempt to set thread real-time priority to the SCHED_FIFO policy
    ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
    if(ret != 0)
    {
        // Print the error
        std::cout << "Unsuccessful in setting thread realtime prio" << std::endl;
        return;
    }

    // Now verify the change in thread priority
    int policy = 0;
    ret = pthread_getschedparam(this_thread, &policy, &params);
    if(ret != 0)
    {
        std::cout << "Couldn't retrieve real-time scheduling paramers" << std::endl;
        return;
    }

    // Check the correct policy was applied
    if(policy != SCHED_FIFO)
        std::cout << "Scheduling is NOT SCHED_FIFO!" << std::endl;
    else
        std::cout << "SCHED_FIFO OK" << std::endl;

    // Print thread scheduling priority
    std::cout << "Thread priority is " << params.sched_priority << std::endl;
}

#elif _WIN32
void max_thread_priority()
{
    std::cout << "max_thread_priority() not implemented for Windows." << std::endl;
}
#endif

} // namespace thread
} // namespace wcore
