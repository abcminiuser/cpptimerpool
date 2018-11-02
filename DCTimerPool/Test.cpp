#include "pch.h"

#include "DCTimerPool.h"


int main()
{
	TimerPool tp;

	auto t = tp.createTimer();
	t->setCallback([]() { printf("TICK!\n"); });
	t->setInterval(std::chrono::milliseconds(1000));
	t->start();

	std::this_thread::sleep_for(std::chrono::seconds(30));
}
