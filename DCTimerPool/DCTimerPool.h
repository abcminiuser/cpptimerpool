#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <forward_list>


class TimerPool : public std::enable_shared_from_this<TimerPool>
{
public:
	class Timer;

	using Clock = std::chrono::steady_clock;
	using Pool = std::shared_ptr<TimerPool>;
	using TimerHandle = std::weak_ptr<Timer>;

	static Pool									CreatePool();

	virtual										~TimerPool();

	void										stop();
	bool										running() const { return m_running; }

	void										wake();

	TimerHandle									createTimer();
	void										deleteTimer(TimerHandle handle);

protected:
	using StrongTimerHandle = std::shared_ptr<Timer>;

	explicit									TimerPool();

	void										run();
	
private:
	std::mutex									m_mutex;
	std::condition_variable						m_cond;
	std::forward_list<StrongTimerHandle>		m_timers;
	bool										m_running;
	std::thread									m_thread;
};

class TimerPool::Timer
{
public:
	explicit									Timer(TimerPool::Pool parentPool);
	virtual										~Timer() = default;

	void										setCallback(std::function<void(void)>&& callback);
	void										setInterval(std::chrono::milliseconds ms);
	void										setRepeated(bool repeated);

	void										start();
	void										stop();

	TimerPool::Clock::time_point				nextExpiry() const;
	TimerPool::Clock::time_point				fire();

private:
	TimerPool::Pool								m_parent;
	mutable std::mutex							m_mutex;
	TimerPool::Clock::time_point				m_nextExpiry;

	bool										m_running;
	std::function<void(void)>					m_callback;
	std::chrono::milliseconds					m_interval;
	bool										m_repeated;
};
