/*
       Thread Safe Timer Pool Library
           By Dean Camera, 2019.

     dean [at] fourwalledcubicle [dot] com
          www.fourwalledcubicle.com
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

#include "TimerPool.hpp"

#include <vector>


namespace
{
    // Private wrapper, used to expose the normally protected constructors
    // internally to the factory methods inside the TimerPool class.
    template <class BaseClass>
    class EnableConstructor final
		: public BaseClass
    {
    public:
        template<typename... Args>
        explicit EnableConstructor(Args&&... args) : BaseClass(std::forward<Args>(args)...) {}

        ~EnableConstructor() = default;
    };

    // This wrapper classes is the reference-counted object that is shared by
    // all created user-timers. It is ref-counted independently to the actual
    // timer instance, so that the timer is automatically registered and
    // unregistered when the first and last user-application timer handle is
    // made. Note that due to the std::share_ptr() aliasing constructor, it will
    // transparently dereference as a normal TimerPool::Timer instance.
    class UserTimer final
    {
    public:
        explicit UserTimer(const TimerPool::TimerHandle& timer)
            : m_timer(timer)
        {
            if (const auto& pool = m_timer->pool())
                pool->registerTimer(timer);
        }

        ~UserTimer()
        {
            m_timer->stop();

            if (const auto& pool = m_timer->pool())
                pool->unregisterTimer(m_timer);
        }

    private:
        const TimerPool::TimerHandle m_timer;
    };
}


TimerPool::PoolHandle TimerPool::Create(const std::string& name)
{
    return std::make_shared<EnableConstructor<TimerPool>>(name);
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

void TimerPool::registerTimer(TimerHandle timer)
{
    {
        std::lock_guard<decltype(m_timerMutex)> timerLock(m_timerMutex);
        std::lock_guard<decltype(m_mutex)>      lock(m_mutex);

        m_timers.remove(timer);
        m_timers.emplace_front(timer);
    }

    m_cond.notify_all();
}

void TimerPool::unregisterTimer(TimerHandle timer)
{
    {
        std::lock_guard<decltype(m_timerMutex)> timerLock(m_timerMutex);
        std::lock_guard<decltype(m_mutex)>      lock(m_mutex);

        m_timers.remove(timer);
    }

    m_cond.notify_all();
}

void TimerPool::run()
{
    std::vector<TimerHandle> expiredTimers;

    while (m_running)
    {
        std::unique_lock<decltype(m_timerMutex)> timerLock(m_timerMutex);
        std::unique_lock<decltype(m_mutex)>      lock(m_mutex);

        const auto nowTime = Clock::now();

        auto wakeTime = nowTime + std::chrono::minutes(1);

        for (const auto& timer : m_timers)
        {
            const auto expiryTime = timer->nextExpiry();

            if (expiryTime <= nowTime)
                expiredTimers.emplace_back(timer);
            else if (expiryTime < wakeTime)
                wakeTime = expiryTime;
        }

        if (! expiredTimers.empty())
        {
            // We fire callbacks without the pool modification lock held, so that the timer callbacks can
            // safely manipulate the pool if desired (and so other threads can change the pool while callbacks
            // are in progress). Note that the timer list modification (recursive) mutex remains held, so that
            // we do block if timers are being (de-)registered while the callbacks are run, so that we don't
            // e.g. try to fire a callback to a partially destroyed user-object that owned the timer being unregistered.

            lock.unlock();

            for (const auto& timer : expiredTimers)
                timer->fire(nowTime);

            expiredTimers.clear();
        }
        else
        {
            // About to enter idle state, release the timer (de-)registration lock so that other threads can (de-)register
            // any timers while the pool is sleeping, when no callbacks are in progress.

            timerLock.unlock();

            m_cond.wait_until(lock, wakeTime);
        }
    }
}

void TimerPool::stop()
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        m_running = false;
        m_timers.clear();
    }

    m_cond.notify_all();
}

void TimerPool::wake()
{
    m_cond.notify_all();
}

// ==================

TimerPool::Timer::TimerHandle TimerPool::Timer::Create(const PoolHandle& pool, const std::string& name)
{
    const auto timer      = std::make_shared<EnableConstructor<Timer>>(pool, name);
    const auto userHandle = std::make_shared<UserTimer>(timer);

    return std::shared_ptr<Timer>(userHandle, timer.get());
}

TimerPool::Timer::Timer(const PoolHandle& pool, const std::string& name)
    : m_pool{ pool }
    , m_name{ name }
    , m_nextExpiry{ Clock::time_point::max() }
    , m_callback{ nullptr }
    , m_interval{ 0 }
    , m_repeated{ false }
{

}

void TimerPool::Timer::setCallback(Callback&& callback)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_callback = callback;
}

void TimerPool::Timer::setInterval(std::chrono::milliseconds ms)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_interval = ms;
}

void TimerPool::Timer::setRepeated(bool repeated)
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    m_repeated = repeated;
}

void TimerPool::Timer::start(StartMode mode)
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        switch (mode)
        {
            case StartMode::StartOnly:
            {
                // Abort if timer already running, we aren't allowing restarts
                if (m_nextExpiry != Clock::time_point::max())
                    return;

                break;
            }

            case StartMode::RestartIfRunning:
            {
                // No preconditons, always (re)start
                break;
            }

            case StartMode::RestartOnly:
            {
                // Abort if timer not already running, we are only allowing restarts
                if (m_nextExpiry == Clock::time_point::max())
                    return;

                break;
            }
        }

        m_nextExpiry = Clock::now() + m_interval;
    }

    if (const auto& pool = m_pool.lock())
        pool->wake();
}

void TimerPool::Timer::stop()
{
    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        m_nextExpiry = Clock::time_point::max();
    }

    if (const auto& pool = m_pool.lock())
        pool->wake();
}

void TimerPool::Timer::fire(Clock::time_point now)
{
    const auto   selfHandle = shared_from_this();
    unsigned int callbacksRequired = 0;
    Callback     callback;

    {
        std::lock_guard<decltype(m_mutex)> lock(m_mutex);

        callback = m_callback;

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

bool TimerPool::Timer::running() const noexcept
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    return m_nextExpiry != Clock::time_point::max();
}

TimerPool::Timer::Clock::time_point TimerPool::Timer::nextExpiry() const noexcept
{
    std::lock_guard<decltype(m_mutex)> lock(m_mutex);

    return m_nextExpiry;
}
