#pragma once

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <functional>
#include <atomic>

class Periodic 
{
public:
    explicit Periodic(const boost::chrono::milliseconds &period, const std::function<void()> &func);

    virtual ~Periodic();

private:
    boost::chrono::milliseconds m_period;
    std::function<void()> m_func;
    std::atomic<bool> m_inFlight;
    boost::thread m_thread;
};