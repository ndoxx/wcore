#include <vector>
#include <algorithm>
#include "moving_average.h"
#include "logger.h"

namespace wcore
{

MovingAverage::MovingAverage(size_t size):
max_size_(size)
{

}

MovingAverage::~MovingAverage() = default;

void MovingAverage::push(float value)
{
    queue_.push_back(value);
    if(queue_.size()>max_size_)
        queue_.pop_front();
}

FinalStatistics MovingAverage::get_stats() const
{
    double mean  = 0.0;
    double variance = 0.0;
    float max_element = 0.0f;
    float min_element = std::numeric_limits<float>::max();
    int n_iter = 0;
    for(float element : queue_)
    {
        if(element>max_element)
            max_element = element;
        else if(element<min_element)
            min_element = element;
        mean += element;
        ++n_iter;
    }
    mean /= n_iter;

    for(auto element : queue_)
    {
        variance += pow((double)element-mean, 2);
    }
    variance /= n_iter;

    // Compute median
    std::vector values(queue_.begin(), queue_.end());
    std::sort(values.begin(), values.end(), [](float xx, float yy)
    {
        return xx<yy;
    });
    float median = 0.0f;
    uint32_t N = values.size();
    if(values.size()%2)
        median = values[(N-1)/2];
    else
        median = 0.5f*(values[N/2-1]+values[N/2]);

    FinalStatistics stats;
    stats.mean    = (float)mean;
    stats.std     = (float)sqrt(variance);
    stats.min_val = min_element;
    stats.max_val = max_element;
    stats.median  = median;

    return stats;
}

void FinalStatistics::debug_print(float scale, const std::string& unit)
{
    DLOGI("Mean value:    <v>" + std::to_string(mean*scale)    + "</v>" + unit);
    DLOGI("Standard dev:  <w>" + std::to_string(std*scale)     + "</w>" + unit);
    DLOGI("Median value:  <w>" + std::to_string(median*scale)  + "</w>" + unit);
    DLOGI("Minimum value: <g>" + std::to_string(min_val*scale) + "</g>" + unit);
    DLOGI("Maximum value: <b>" + std::to_string(max_val*scale) + "</b>" + unit);
}

}
