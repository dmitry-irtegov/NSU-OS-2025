CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wpedantic
SRC=main.cpp
HEADERS=posix_process.hpp thirdparty/include/expected.hpp

all: launch_nowait launch_wait

clean:
	rm -f launch_nowait launch_wait

launch_wait: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ 

launch_nowait: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -DNOWAIT $(SRC) -o $@ 
