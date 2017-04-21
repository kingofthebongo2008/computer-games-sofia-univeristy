#pragma once

#include <cstdint>
#include <windows.h>


namespace sys
{
    /// Create a Timer, which will immediately begin counting
    /// up from 0.0 seconds.
    /// You can call reset() to make it start over.
    class profile_timer
    {
    public:
        profile_timer()
        {
            reset();
        }

        /// reset() makes the timer start over counting from 0.0 seconds.
        void reset()
        {
            uint64_t pf = 1;
            QueryPerformanceFrequency((LARGE_INTEGER *)&pf);
            m_freq = 1.0 / (double)pf;
            m_base_time = 0;
            QueryPerformanceCounter((LARGE_INTEGER *)&m_base_time);
        }

        /// seconds() returns the number of seconds (to very high resolution)
        /// elapsed since the timer was last created or reset().
        double seconds() const
        {
            uint64_t val = m_base_time;
            QueryPerformanceCounter((LARGE_INTEGER *)&val);
            return (val - m_base_time) * m_freq;
        }

        /// seconds() returns the number of milliseconds (to very high resolution)
        /// elapsed since the timer was last created or reset().
        double milliseconds() const
        {
            return seconds() * 1000.0;
        }

    private:
        double      m_freq;
        uint64_t    m_base_time;
    };

}



