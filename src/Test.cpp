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
    static const auto kPrintTimerCallback =
        [](TimerPool::TimerHandle t)
        {
            const auto poolName  = t->pool().lock()->name();
            const auto timerName = t->name();

            printf("%s - %s\n", poolName.c_str(), timerName.c_str());
        };

    // TEST 1: Timer pool is long lived, two long lived timers (both should run)
    auto pool1 = TimerPool::CreatePool("Pool 1");

    auto timer1 = pool1->createTimer("TICK!");
    timer1->setCallback(kPrintTimerCallback);
    timer1->setInterval(std::chrono::seconds(1));
    timer1->setRepeated(true);
    timer1->start();

    auto timer2 = pool1->createTimer("TOCK!");
    timer2->setCallback(kPrintTimerCallback);
    timer2->setInterval(std::chrono::milliseconds(250));
    timer2->setRepeated(true);
    timer2->start();

    // TEST 2: Timer pool is very long lived, two timers whose references are discarded (both should still run)
    {
        static auto pool2 = TimerPool::CreatePool("Pool 2");

        auto timer3 = pool2->createTimer("Alpha");
        timer3->setCallback(kPrintTimerCallback);
        timer3->setInterval(std::chrono::milliseconds(666));
        timer3->setRepeated(true);
        timer3->start();

        if (auto timer4 = pool2->createTimer("Beta"))
        {
            timer4->setCallback(kPrintTimerCallback);
            timer4->setInterval(std::chrono::milliseconds(333));
            timer4->setRepeated(true);
            timer4->start();
        }
    }

    // TEST 3: Timer is created after its parent pool is discarded (should not run)
    {
        TimerPool::TimerHandle timer5;
        {
            auto pool3 = TimerPool::CreatePool("Pool 3");

            timer5 = pool3->createTimer("Discarded Parent Pool 3 Timer");
        }

        timer5->setCallback(kPrintTimerCallback);
        timer5->setInterval(std::chrono::milliseconds(100));
        timer5->setRepeated(true);
        timer5->start();
    }

    // TEST 4: Timer is created and handle retained, but parent pool is only weakly retained and so is discarded (should not run)
    {
        static TimerPool::WeakPoolHandle pool4weak;
        static TimerPool::TimerHandle timer6;
        {
            auto pool4 = TimerPool::CreatePool("Pool 5");

            timer6 = pool4->createTimer("Discarded Parent Pool 4 Timer");

            pool4weak = pool4;
        }

        timer6->setCallback(kPrintTimerCallback);
        timer6->setInterval(std::chrono::milliseconds(100));
        timer6->setRepeated(true);
        timer6->start();
    }

	// TEST 5: Timer and its parent pool are retained, but the pool is manually stopped before it can run (should not run)
	auto pool5 = TimerPool::CreatePool("Pool 1");
	pool5->stop();

	auto timer7 = pool5->createTimer("Stopped Parent Pool 5 Timer");
	timer7->setCallback(kPrintTimerCallback);
	timer7->setInterval(std::chrono::seconds(1));
	timer7->setRepeated(true);
	timer7->start();

    std::this_thread::sleep_for(std::chrono::seconds(10));
}
