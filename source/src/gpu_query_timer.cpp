#include <GL/glew.h>
#include "gpu_query_timer.h"

#include <iostream>
namespace wcore
{

GPUQueryTimer::GPUQueryTimer():
query_back_buffer_(0),
query_front_buffer_(1)
{
    query_ID_ = new unsigned int[2];

    glGenQueries(1, &query_ID_[query_back_buffer_]);
    glGenQueries(1, &query_ID_[query_front_buffer_]);

    // dummy query to prevent OpenGL errors from popping out during first frame
    //glQueryCounter(query_ID_[query_front_buffer_], GL_TIMESTAMP);
}

GPUQueryTimer::~GPUQueryTimer()
{
    delete[] query_ID_;
}

void GPUQueryTimer::start()
{
    glBeginQuery(GL_TIME_ELAPSED, query_ID_[query_back_buffer_]);
}

float GPUQueryTimer::stop()
{
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectuiv(query_ID_[query_front_buffer_], GL_QUERY_RESULT, (GLuint*)&timer_);
    swap_query_buffers();

    return timer_ / 1000000000.f;
}

void GPUQueryTimer::swap_query_buffers()
{
    if(query_back_buffer_)
    {
        query_back_buffer_ = 0;
        query_front_buffer_ = 1;
    }
    else
    {
        query_back_buffer_ = 1;
        query_front_buffer_ = 0;
    }
}

} // namespace wcore
