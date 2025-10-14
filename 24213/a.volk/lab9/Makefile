CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wpedantic -I/Users/sxrose/thirdparty/include -I/home/students/24200/a.volk/thirdparty/include
SRC=main.cpp
HEADERS=posix_process.hpp

all: launch_nowait launch_wait

clean:
	rm -f launch_nowait launch_wait

launch_wait: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ 

launch_nowait: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -DNOWAIT $(SRC) -o $@ 
