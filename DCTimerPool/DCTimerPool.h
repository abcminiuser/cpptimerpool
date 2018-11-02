#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <forward_list>


class TimerPool
{
public:
	explicit									TimerPool();
	virtual										~TimerPool();

	bool										running() const { return m_run; }

	class Timer;
	using TimerHandle = std::shared_ptr<Timer>;

	using Clock = std::chrono::steady_clock;

	TimerHandle									createTimer();

private:
	void										run();
	void										stop();
	
private:
	std::mutex									m_mutex;
	std::atomic<bool>							m_run;
	std::condition_variable						m_cond;
	std::thread									m_thread;
	std::forward_list<TimerHandle>				m_timers;
};

class TimerPool::Timer
{
public:
	explicit									Timer(TimerPool& parentPool)
		: m_parent(parentPool)
		, m_running(false)
	{}

	virtual										~Timer() = default;

	void										setCallback(std::function<void(void)>&& callback)
	{
		m_callback = callback;
	}

	void										setInterval(std::chrono::milliseconds ms)
	{
		m_interval = ms;
	}

	void										setRepeated(bool repeated)
	{
		m_repeated = repeated;
	}

	void										start()
	{
		m_running = true;
		m_nextExpiry = TimerPool::Clock::now() + m_interval;
	}

	void										stop()
	{
		m_running = false;
	}

	void										fire()
	{
		if (m_callback)
			m_callback();

		m_nextExpiry = TimerPool::Clock::now() + m_interval;
	}

	TimerPool::Clock::time_point nextExpiry() const
	{
		if (! m_running)
			return TimerPool::Clock::time_point::max();

		return m_nextExpiry;
	}

private:
	TimerPool&						m_parent;
	std::function<void(void)>		m_callback;
	std::chrono::milliseconds		m_interval;
	bool							m_repeated;

	bool							m_running;
	TimerPool::Clock::time_point	m_nextExpiry;
};
