#pragma once

#include <condition_variable>
#include <forward_list>
#include <functional>
#include <memory>
#include <string>
#include <thread>


class TimerPool
	: public std::enable_shared_from_this<TimerPool>
{
public:
	class Timer;

	using Clock = std::chrono::steady_clock;
	using PoolHandle = std::shared_ptr<TimerPool>;
	using WeakTimerHandle = std::weak_ptr<Timer>;

	static PoolHandle				CreatePool(const std::string& name = "");

	virtual							~TimerPool();

	void							stop();
	std::string						name() const	{ return m_name; }
	bool							running() const { return m_running; }

	void							wake();

	WeakTimerHandle					createTimer();
	void							deleteTimer(WeakTimerHandle handle);

protected:
	using TimerHandle = std::shared_ptr<Timer>;

	explicit						TimerPool(const std::string& name);

	void							run();

private:
	mutable std::mutex				m_mutex;
	std::condition_variable			m_cond;

	const std::string				m_name;
	std::forward_list<TimerHandle>	m_timers;

	bool							m_running;
	std::thread						m_thread;
};

class TimerPool::Timer
{
public:
	using Clock = TimerPool::Clock;
	using Callback = std::function<void(void)>;
	using PoolHandle = TimerPool::PoolHandle;

	explicit									Timer(PoolHandle parentPool);
	virtual										~Timer() = default;

	void										setCallback(Callback&& callback);
	void										setInterval(std::chrono::milliseconds ms);
	void										setRepeated(bool repeated);

	void										start();
	void										stop();

	Clock::time_point							nextExpiry() const;
	Clock::time_point							fire();

private:
	mutable std::mutex							m_mutex;
	PoolHandle									m_parent;
	Clock::time_point							m_nextExpiry;

	bool										m_running;
	Callback									m_callback;
	std::chrono::milliseconds					m_interval;
	bool										m_repeated;
};
