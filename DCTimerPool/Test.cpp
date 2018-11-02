#include "pch.h"

#include "DCTimerPool.h"


int main()
{
	TimerPool tp;

	auto handle = tp.createTimer();
	if (auto t = handle.lock())
	{
		t->setCallback([]() { printf("TICK!\n"); });
		t->setInterval(std::chrono::milliseconds(1000));
		t->start();
	}

	std::this_thread::sleep_for(std::chrono::seconds(30));
}
