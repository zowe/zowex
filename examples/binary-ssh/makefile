CXX=ibm-clang++64
CXXFLAGS=-std=c++17

all: server tests/testb64 tests/testb85 tests/testraw

server: server.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

tests/testb64: tests/testb64.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

tests/testb85: tests/testb85.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

tests/testraw: tests/testraw.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f server tests/testb64 tests/testb85 tests/testraw
