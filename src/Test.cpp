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

#include <iostream>
#include <sstream>

int main()
{
	static const auto kPrintTimerCallback =
		[](const TimerPool::TimerHandle& t)
		{
			const auto poolName  = t->pool()->name();
			const auto timerName = t->name();

			std::stringstream message;
			message << poolName << " - " << timerName << "\n";

			std::cout << message.str();
		};

	// TEST 1: Timer pool is long lived, two long lived timers (both should run)
	auto pool1 = TimerPool::Create("Pool 1");

	auto timer1 = TimerPool::Timer::Create(pool1, "TICK!");
	timer1->setCallback(kPrintTimerCallback);
	timer1->setInterval(std::chrono::seconds(1));
	timer1->setRepeated(true);
	timer1->start();

	auto timer2 = TimerPool::Timer::Create(pool1, "TOCK!");
	timer2->setCallback(kPrintTimerCallback);
	timer2->setInterval(std::chrono::milliseconds(250));
	timer2->setRepeated(true);
	timer2->start();

	// TEST 2: Timer pool is very long lived, two timers which are also long lived (both should run)
	{
		static auto pool2 = TimerPool::Create("Pool 2");

		static auto timer3 = TimerPool::Timer::Create(pool2, "Alpha");
		timer3->setCallback(kPrintTimerCallback);
		timer3->setInterval(std::chrono::milliseconds(666));
		timer3->setRepeated(true);
		timer3->start();

		static auto timer4 = TimerPool::Timer::Create(pool2, "Beta");
		timer4->setCallback(kPrintTimerCallback);
		timer4->setInterval(std::chrono::milliseconds(333));
		timer4->setRepeated(true);
		timer4->start();
	}

	// TEST 3: Timer is created before its parent pool is discarded (should not run)
	{
		TimerPool::TimerHandle timer5;
		{
			auto pool3 = TimerPool::Create("Pool 3");

			timer5 = TimerPool::Timer::Create(pool3, "Discarded Parent Pool 3 Timer");
		}

		timer5->setCallback(kPrintTimerCallback);
		timer5->setInterval(std::chrono::milliseconds(100));
		timer5->setRepeated(true);
		timer5->start();
	}

	// TEST 4: Timer is created and handle retained, but parent pool is only weakly retained and so is discarded (should not run)
	{
		static TimerPool::WeakPoolHandle pool4weak;
		static TimerPool::TimerHandle    timer6;

		{
			auto pool4 = TimerPool::Create("Pool 4");

			timer6 = TimerPool::Timer::Create(pool4, "Discarded Parent Pool 4 Timer");

			pool4weak = pool4;
		}

		timer6->setCallback(kPrintTimerCallback);
		timer6->setInterval(std::chrono::milliseconds(100));
		timer6->setRepeated(true);
		timer6->start();
	}

	// TEST 5: Timer and its parent pool are retained, but the pool is manually stopped before it can run (should not run)
	auto pool5 = TimerPool::Create("Pool 5");
	pool5->stop();

	auto timer7 = TimerPool::Timer::Create(pool5, "Stopped Parent Pool 5 Timer");
	timer7->setCallback(kPrintTimerCallback);
	timer7->setInterval(std::chrono::seconds(1));
	timer7->setRepeated(true);
	timer7->start();

	// TEST 6: Parent pool is very long lived, timer's only user reference is discarded (should not run)
	auto pool6 = TimerPool::Create("Pool 6");

	if (auto timer8 = TimerPool::Timer::Create(pool6, "Discarded Pool 6 Timer"))
	{
		timer8->setCallback(kPrintTimerCallback);
		timer8->setInterval(std::chrono::seconds(1));
		timer8->setRepeated(true);
		timer8->start();
	}

	// TEST 7: Pool is very long lived, but timer is only weakly retained (should not run)
	{
		static TimerPool::PoolHandle pool7 = TimerPool::Create("Pool 7");
		TimerPool::WeakTimerHandle   timer9weak;

		if (auto timer9 = TimerPool::Timer::Create(pool7, "Weak Pool 7 Timer"))
		{
			timer9->setCallback(kPrintTimerCallback);
			timer9->setInterval(std::chrono::seconds(1));
			timer9->setRepeated(true);
			timer9->start();

			timer9weak = timer9;
		}
	}

	// TEST 8: Pool is very long lived, timer is retained strongly by copy (should run)
	{
		static TimerPool::PoolHandle  pool8 = TimerPool::Create("Pool 8");
		static TimerPool::TimerHandle timer10strong;

		if (auto timer10 = TimerPool::Timer::Create(pool8, "GAMMA"))
		{
			timer10->setCallback(kPrintTimerCallback);
			timer10->setInterval(std::chrono::milliseconds(400));
			timer10->setRepeated(true);
			timer10->start();

			timer10strong = timer10;
		}
	}

	std::this_thread::sleep_for(std::chrono::seconds(10));
}
