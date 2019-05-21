#ifndef GPU_QUERY_TIMER_H
#define GPU_QUERY_TIMER_H

namespace wcore
{

class GPUQueryTimer
{
public:
    GPUQueryTimer();
    ~GPUQueryTimer();

    // Start query timer
    void start();
    // Stop timer and get elapsed GPU time in s
    float stop();

private:
    // Helper func to swap query buffers
    void swap_query_buffers();

private:
    // the array to store the two sets of queries.
    unsigned int* query_ID_;
    unsigned int query_back_buffer_;
    unsigned int query_front_buffer_;
    uint32_t timer_;
};

} // namespace wcore

#endif
