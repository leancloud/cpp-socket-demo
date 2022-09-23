cpp-socket: $(wildcard *.cpp)
	g++ -std=c++11 -pthread -o $@ $^
