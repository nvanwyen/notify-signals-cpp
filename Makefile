all: test-dir

test-dir: main.cpp dir.hpp dir.cpp
	@g++ -o test-dir main.cpp dir.cpp -lboost_system -lboost_thread -lboost_filesystem -lboost_regex

clean:
	@rm -f test-dir *.o
