all:
	$(CXX) -Wall -Wextra --std=c++14 -pthread *.cpp -o test

test: all
	./test

cppcheck:
	cppcheck --quiet --enable=all --suppress=unusedFunction --suppress=missingIncludeSystem .

valgrind: all
	valgrind ./test

.PHONY: all test cppcheck valgrind
