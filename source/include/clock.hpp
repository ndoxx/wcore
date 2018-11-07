#ifndef CLOCK_HPP_INCLUDED
#define CLOCK_HPP_INCLUDED

#include <chrono>

template < typename Period >
class Clock
{
public:
    Clock();
    Period get_elapsed_time() const;
    Period restart();

private:
    std::chrono::time_point< std::chrono::high_resolution_clock > time_point_;
};

template < typename Period >
Clock< Period >::Clock()
    : time_point_( std::chrono::high_resolution_clock::now() )
{}

template < typename Period >
Period Clock< Period >::get_elapsed_time() const
{
    const Period elapsedTime( std::chrono::high_resolution_clock::now() - time_point_ );
    return elapsedTime;
}

template < typename Period >
Period Clock< Period >::restart()
{
    const Period period( get_elapsed_time() );
    time_point_ = std::chrono::high_resolution_clock::now();
    return period;
}

using nanoClock = Clock<std::chrono::nanoseconds>;

#ifdef __DEBUG__
namespace dbg
{
static nanoClock debug_clock;
}

inline void TIC_()
{
    dbg::debug_clock.restart();
}

inline float TOC_()
{
    auto period = dbg::debug_clock.get_elapsed_time();
    return std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
}
#endif

#endif // CLOCK_HPP_INCLUDED
