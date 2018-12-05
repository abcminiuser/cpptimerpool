all:
	$(CXX) -Wall -Wextra --std=c++14 -pthread *.cpp -o test

test: all
	./test

valgrind: all
	valgrind ./test

.PHONY: all test valgrind
