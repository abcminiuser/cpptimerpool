#include "DCTimerPool.h"


int main()
{
	auto pool = TimerPool::CreatePool();

	auto handle = pool->createTimer();
	if (auto t = handle.lock())
	{
		t->setCallback([]() { printf("TICK!\n"); });
		t->setInterval(std::chrono::milliseconds(1000));
		t->start();
	}

	auto handle2 = pool->createTimer();
	if (auto t = handle2.lock())
	{
		t->setCallback([]() { printf("TOCK!\n"); });
		t->setInterval(std::chrono::milliseconds(100));
		t->start();
	}

	std::this_thread::sleep_for(std::chrono::seconds(30));
}
