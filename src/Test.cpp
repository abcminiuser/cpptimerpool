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


int main()
{
	const auto kPrintTimerCallback = [](TimerPool::Timer::TimerHandle t) { printf("%s - %s\n", t->pool().lock()->name().c_str(), t->name().c_str()); };

	// Strong, long lived pool, two weak timers (both should run)
	auto pool1 = TimerPool::CreatePool("Pool 1");

	auto handle1 = pool1->createTimer("TICK!");
	if (auto t = handle1.lock())
	{
		t->setCallback(kPrintTimerCallback);
		t->setInterval(std::chrono::milliseconds(1000));
		t->setRepeated(true);
		t->start();
	}

	auto handle2 = pool1->createTimer("TOCK!");
	if (auto t = handle2.lock())
	{
		t->setCallback(kPrintTimerCallback);
		t->setInterval(std::chrono::milliseconds(250));
		t->setRepeated(true);
		t->start();
	}

	// Strong, very long lived pool, two weak timers (both should run)
	{
		static auto pool2 = TimerPool::CreatePool("Pool 2");

		auto handle3 = pool2->createTimer("Alpha");
		if (auto t = handle3.lock())
		{
			t->setCallback(kPrintTimerCallback);
			t->setInterval(std::chrono::milliseconds(666));
			t->setRepeated(true);
			t->start();
		}

		auto handle4 = pool2->createTimer("Beta");
		if (auto t = handle4.lock())
		{
			t->setCallback(kPrintTimerCallback);
			t->setInterval(std::chrono::milliseconds(333));
			t->setRepeated(true);
			t->start();
		}
	}

	// Weak, short-lived pool, weak timer that outlives its pool (should never run)
	{
		TimerPool::WeakTimerHandle handle5;
		{
			auto pool3 = TimerPool::CreatePool("Pool 3");

			handle5 = pool3->createTimer("Weak Timer");
		}

		if (auto t = handle5.lock())
		{
			t->setCallback(kPrintTimerCallback);
			t->setInterval(std::chrono::milliseconds(100));
			t->setRepeated(true);
			t->start();
		}
	}

	// Weak, short-lived pool, strong timer that outlives its pool (should never run)
	{
		TimerPool::TimerHandle t;
		{
			auto pool4 = TimerPool::CreatePool("Pool 4");

			t = pool4->createTimer("Strong Timer").lock(); // Strong ref
		}

		if (t)
		{
			t->setCallback(kPrintTimerCallback);
			t->setInterval(std::chrono::milliseconds(100));
			t->setRepeated(true);
			t->start();
		}
	}

	// Weak, long-lived pool, strong timer that outlives its pool (should never run)
	{
		static TimerPool::TimerHandle t;
		static TimerPool::WeakPoolHandle p;

		{
			auto pool5 = TimerPool::CreatePool("Pool 5");

			t = pool5->createTimer("Strong Timer").lock(); // Strong ref
			p = t->pool(); // Weak ref
		}

		if (t)
		{
			t->setCallback(kPrintTimerCallback);
			t->setInterval(std::chrono::milliseconds(100));
			t->setRepeated(true);
			t->start();
		}
	}

	std::this_thread::sleep_for(std::chrono::seconds(30));
}
