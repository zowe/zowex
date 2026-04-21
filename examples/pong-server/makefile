CXX=ibm-clang++64
CXXFLAGS=-std=c++17

all: server

server: server.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f server
