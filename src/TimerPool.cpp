/*
       Thread Safe Timer Pool Library
       Copyright (C) Dean Camera, 2018.

     dean [at] fourwalledcubicle [dot] com
          www.fourwalledcubicle.cpm
*/

/*
    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org/>
*/

#include "TimerPool.h"


TimerPool::PoolHandle TimerPool::CreatePool(const std::string& name)
{
    return std::shared_ptr<TimerPool>(new TimerPool(name));
}

TimerPool::TimerPool(const std::string& name)
    : m_mutex{ }
    , m_name{ name }
    , m_timers{ }
    , m_running{ true }
    , m_cond{ }
    , m_thread{ [this]() { run(); } }
{

}

TimerPool::~TimerPool()
{
    stop();

    if (m_thread.joinable())
        m_thread.join();
}

void TimerPool::wake()
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);
    m_cond.notify_all();
}

TimerPool::TimerHandle TimerPool::createTimer(const std::string& name)
{
    auto timer = std::make_shared<Timer>(shared_from_this(), name);

    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);
        m_timers.remove(timer);
        m_timers.emplace_front(timer);
    }

    return timer;
}

void TimerPool::deleteTimer(TimerHandle timer)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_timers.remove(timer);
    m_cond.notify_all();
}

void TimerPool::run()
{
    while (m_running)
    {
        std::unique_lock<decltype(m_mutex)> lock(m_mutex);

        auto nowTime  = Clock::now();
        auto wakeTime = Clock::time_point::max();

        for (const auto& t : m_timers)
        {
            auto expiryTime = t->nextExpiry();

            if (nowTime >= expiryTime)
                expiryTime = t->fire();

            if (expiryTime < wakeTime)
                wakeTime = expiryTime;
        }

        m_cond.wait_until(lock, wakeTime);
    }
}

void TimerPool::stop()
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_running = false;
    m_timers.clear();

    m_cond.notify_all();
}

// ==================

TimerPool::Timer::Timer(WeakPoolHandle pool, const std::string& name)
    : m_pool{ pool }
    , m_name{ name }
    , m_nextExpiry{ Clock::time_point::max() }
    , m_running{ false }
    , m_callback{ nullptr }
    , m_interval{ 0 }
    , m_repeated{ false }
{

}

void TimerPool::Timer::setCallback(Callback&& callback)
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        m_callback = callback;
        m_nextExpiry = Clock::now() + m_interval;
    }

    if (auto pool = m_pool.lock())
        pool->wake();
}

void TimerPool::Timer::setInterval(std::chrono::milliseconds ms)
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        m_interval = ms;
        m_nextExpiry = Clock::now() + m_interval;
    }

    if (auto pool = m_pool.lock())
        pool->wake();
}

void TimerPool::Timer::setRepeated(bool repeated)
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        m_repeated = repeated;
        m_nextExpiry = Clock::now() + m_interval;
    }

    if (auto pool = m_pool.lock())
        pool->wake();
}

void TimerPool::Timer::start()
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        m_running = true;
        m_nextExpiry = Clock::now() + m_interval;
    }

    if (auto pool = m_pool.lock())
        pool->wake();
}

void TimerPool::Timer::stop()
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        m_running = false;
    }

    if (auto pool = m_pool.lock())
        pool->wake();
}

TimerPool::Timer::Clock::time_point TimerPool::Timer::fire()
{
    decltype(m_callback)   callback;
    decltype(m_nextExpiry) nextExpiry;

    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        if (m_repeated)
            m_nextExpiry = Clock::now() + m_interval;
        else
            m_nextExpiry = Clock::time_point::max();

        callback = m_callback;
        nextExpiry = m_nextExpiry;
    }

    if (callback)
        callback(shared_from_this());

    return nextExpiry;
}

TimerPool::Timer::Clock::time_point TimerPool::Timer::nextExpiry() const
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    if (! m_running)
        return Clock::time_point::max();

    return m_nextExpiry;
}
