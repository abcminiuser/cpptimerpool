Thread-Safe Timer Pool
========================


Overview
---------------

This is a simple C++14 source library for creating multiple timer pools, each of
which can contain multiple timers with configurable timeout intervals and
callback functions.

I've made use of shared/weak pointers to ensure that the timers do not leak and
that handles to timers created within a pool do not outlive their parent.

Callbacks are executed on the pool's thread that created the timer. Timer
intervals are specified in milliseconds, based on wall-clock time (using the
standard libraries' `std::chrono::steady_clock` as the timer pool's timer
reference).


Object Lifespan
----------------

Created timers are weakly bound to their parent pools; if the last user-held
reference to the parent pool expires, the pool is destroyed and all child timers
will cease to run.

Timers will continue to run if their last user-held reference expires; they are
strongly owned by their parent pool.

A timer object whose parent pool is no longer valid will not run, however it
will remain a valid object and can be safely modified without deadlocks or
crashes.


Compatibility
----------------

Tested on Visual Studio 2017 (Windows) and GCC (Linux). Only C++14 standard
library and compiler support is required, no special libraries, although on
Posix systems this generally assumes `pthreads` is available.


License
----------------

This library is released into the public domain, however patches are welcome
(and appreciated!). The full license text is as follows:

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
