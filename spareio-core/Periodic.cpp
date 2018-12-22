#include "Periodic.h"

Periodic::Periodic(const boost::chrono::milliseconds &period, const std::function<void()> &func) :
    m_period(period), 
    m_func(func), 
    m_inFlight(true)
{
    m_thread = boost::thread([this]
    {
        try
        {
            while (m_inFlight) 
            {
                boost::this_thread::sleep_for(m_period);
                if (m_inFlight) 
                {
                    m_func();
                }
            }
        }
        catch (boost::thread_interrupted&) {}
    });
}

Periodic::~Periodic() 
{
    m_inFlight = false;
    m_thread.interrupt();
    m_thread.join();
}
