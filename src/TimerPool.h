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

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <forward_list>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>


class TimerPool
    : public std::enable_shared_from_this<TimerPool>
{
public:
    class Timer;

    using Clock           = std::chrono::steady_clock;
    using WeakPoolHandle  = std::weak_ptr<TimerPool>;
    using PoolHandle      = std::shared_ptr<TimerPool>;
    using WeakTimerHandle = std::weak_ptr<Timer>;
    using TimerHandle     = std::shared_ptr<Timer>;

public:
    static PoolHandle               Create(const std::string& name = "");

    virtual                         ~TimerPool();

    std::string                     name() const    { return m_name; }
    bool                            running() const { return m_running; }

    void                            stop();
    void                            wake();

    void                            registerTimer(TimerHandle timer);
    void                            unregisterTimer(TimerHandle timer);

protected:
    explicit                        TimerPool(const std::string& name);

                                    TimerPool(const TimerPool&) = delete;
    TimerPool&                      operator=(const TimerPool&) = delete;

    void                            run();

private:
    mutable std::mutex              m_mutex;

    const std::string               m_name;

    std::mutex						m_timerMutex;
    std::forward_list<TimerHandle>  m_timers;

    std::atomic<bool>               m_running;

    std::condition_variable         m_cond;
    std::thread                     m_thread;
};

class TimerPool::Timer
    : public std::enable_shared_from_this<Timer>
{
public:
    using Clock           = TimerPool::Clock;
    using WeakPoolHandle  = TimerPool::WeakPoolHandle;
    using PoolHandle      = TimerPool::PoolHandle;
    using WeakTimerHandle = TimerPool::WeakTimerHandle;
    using TimerHandle     = TimerPool::TimerHandle;
    using Callback        = std::function<void(TimerHandle)>;

public:
    static TimerHandle              Create(PoolHandle pool, const std::string& name = "");

    virtual                         ~Timer() = default;

    WeakPoolHandle                  pool() const { return m_pool; }

    std::string                     name() const { return m_name; }

    void                            setCallback(Callback&& callback);
    void                            setInterval(std::chrono::milliseconds ms);
    void                            setRepeated(bool repeated);

    enum class StartMode
    {
        StartOnly,
        RestartIfRunning,
        RestartOnly,
    };

    void                            start(StartMode mode = StartMode::RestartIfRunning);
    void                            stop();

    bool                            running() const noexcept;
    Clock::time_point               nextExpiry() const noexcept;

    void                            fire(Clock::time_point now = Clock::time_point::min());

protected:
    explicit                        Timer(PoolHandle pool, const std::string& name = "");

                                    Timer(const Timer&) = delete;
    Timer&                          operator=(const Timer&) = delete;

private:
    mutable std::mutex              m_mutex;

    const WeakPoolHandle            m_pool;
    const std::string               m_name;

    Clock::time_point               m_nextExpiry;

    bool                            m_running;
    Callback                        m_callback;
    std::chrono::milliseconds       m_interval;
    bool                            m_repeated;
};
