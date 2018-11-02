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
	class Timer;

	using Clock = std::chrono::steady_clock;
	using TimerHandle = std::weak_ptr<Timer>;

	explicit									TimerPool();
	virtual										~TimerPool();

	void										stop();
	bool										running() const { return m_run; }

	void										wake();

	TimerHandle									createTimer();
	void										deleteTimer(TimerHandle handle);

private:
	using StrongTimerHandle = std::shared_ptr<Timer>;

	void										run();
	
private:
	std::mutex									m_mutex;
	std::condition_variable						m_cond;
	std::thread									m_thread;
	std::forward_list<StrongTimerHandle>		m_timers;
	bool										m_run;
};

class TimerPool::Timer
{
public:
	explicit									Timer(TimerPool& parentPool);
	virtual										~Timer() = default;

	void										setCallback(std::function<void(void)>&& callback);
	void										setInterval(std::chrono::milliseconds ms);
	void										setRepeated(bool repeated);
	void										start();
	void										stop();

	TimerPool&									parent() const { return m_parent; }
	TimerPool::Clock::time_point				nextExpiry() const;
	TimerPool::Clock::time_point				fire();

private:
	TimerPool&									m_parent;
	mutable std::mutex							m_mutex;
	TimerPool::Clock::time_point				m_nextExpiry;

	bool										m_running;
	std::function<void(void)>					m_callback;
	std::chrono::milliseconds					m_interval;
	bool										m_repeated;
};
