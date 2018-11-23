#ifndef MOVING_AVERAGE_H
#define MOVING_AVERAGE_H

#include <cstdint>
#include <deque>

#include "math3d.h"

namespace wcore
{

struct FinalStatistics
{
public:
    FinalStatistics():
    mean(0),
    std(0),
    min_val(std::numeric_limits<float>::max()),
    max_val(0),
    median(0)
    {}

    void debug_print(float scale, const std::string& unit, const char* channel="default");

    float mean;
    float std;
    float min_val;
    float max_val;
    float median;
};

class MovingAverage
{
protected:
    std::deque<float> queue_;
    size_t max_size_;

public:
    MovingAverage(size_t size=100);
    ~MovingAverage();

    void push(float value);
    FinalStatistics get_stats() const;

    inline float last_element() const { return queue_.back(); }
    inline size_t get_size() const    { return std::min(queue_.size(), max_size_); }
};

}

#endif // MOVING_AVERAGE_H
