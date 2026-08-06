CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -Wpedantic -g

SRC=main.cpp
EXE=getline

all: $(EXE)

clean:
	rm -f $(EXE)

$(EXE): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ 
