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


namespace
{
	class TimerPoolPrivate : public TimerPool
	{
	public:
		template<typename... Args>
		TimerPoolPrivate(Args&&... args) : TimerPool(std::forward<Args>(args)...) {}

		virtual ~TimerPoolPrivate() = default;
	};

	class TimerPrivate : public TimerPool::Timer
	{
	public:
		template<typename... Args>
		TimerPrivate(Args&&... args) : Timer(std::forward<Args>(args)...) {}

		virtual ~TimerPrivate() = default;
	};

	// This wrapper classes is the reference-counted object that is shared by
	// all created user-timers. It's ref-counted independently to the actual
	// timer instance, so that the timer is automatically registered and
	// unregistered when the first and last user-application timer handle is
	// made. Note that due to the std::share_ptr() aliasing constructor, it will
	// transparently dereference as a normal TimerPool::Timer instance.
	class UserTimer
	{
	public:
		explicit UserTimer(TimerPool::TimerHandle timer)
			: m_timer(timer)
		{
			if (auto pool = m_timer->pool().lock())
				pool->registerTimer(timer);
		}

		virtual ~UserTimer()
		{
			m_timer->stop();

			if (auto pool = m_timer->pool().lock())
				pool->unregisterTimer(m_timer);
		}

	private:
		TimerPool::TimerHandle m_timer;
	};
}


TimerPool::PoolHandle TimerPool::Create(const std::string& name)
{
    return std::make_shared<TimerPoolPrivate>(name);
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

void TimerPool::registerTimer(TimerHandle timer)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_timers.remove(timer);
    m_timers.emplace_front(timer);
}

void TimerPool::unregisterTimer(TimerHandle timer)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_timers.remove(timer);
    m_cond.notify_all();
}

void TimerPool::run()
{
    std::vector<TimerHandle> expiredTimers;

    while (m_running)
    {
        std::unique_lock<decltype(m_mutex)> lock(m_mutex);

        auto nowTime  = Clock::now();
        auto wakeTime = Clock::time_point::max();

        expiredTimers.clear();

        for (const auto& timer : m_timers)
        {
            auto expiryTime = timer->nextExpiry();

            if (expiryTime <= nowTime)
                expiredTimers.push_back(timer);
            else if (expiryTime < wakeTime)
                wakeTime = expiryTime;
        }

        if (! expiredTimers.empty())
        {
            lock.unlock();

            for (const auto& timer : expiredTimers)
                timer->fire(nowTime);
        }
        else
        {
            m_cond.wait_until(lock, wakeTime);
        }
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

TimerPool::Timer::TimerHandle TimerPool::Timer::Create(PoolHandle pool, const std::string& name)
{
    auto timer      = std::make_shared<TimerPrivate>(pool, name);
    auto userHandle = std::make_shared<UserTimer>(timer);

    return std::shared_ptr<Timer>(userHandle, timer.get());
}

TimerPool::Timer::Timer(PoolHandle pool, const std::string& name)
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

void TimerPool::Timer::fire(Clock::time_point now)
{
	size_t		callbacksRequired = 0;
	Callback    callback;
	TimerHandle selfHandle;

    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

		selfHandle = shared_from_this();
		callback   = m_callback;

		if (m_repeated)
		{
			// We might have to catch up to the current time - it's more efficient
			// to fire as many callbacks as we can be sure we've missed right now while
			// we're making expensive callback object copies, then clean up any extra
			// missed callbacks later when the parent pool re-evaluates the pool timers.
			do
			{
				m_nextExpiry += m_interval;
				callbacksRequired++;
			} while (m_nextExpiry < now);
		}
		else
		{
			m_nextExpiry = Clock::time_point::max();
			callbacksRequired++;
		}
    }

	if (callback)
	{
		while (callbacksRequired--)
			callback(selfHandle);
	}
}

TimerPool::Timer::Clock::time_point TimerPool::Timer::nextExpiry() const
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    if (! m_running)
        return Clock::time_point::max();

    return m_nextExpiry;
}
