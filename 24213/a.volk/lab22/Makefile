CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -Wpedantic -g

CXXFLAGSEXTRA=

SRC=main.cpp
EXE=multiplex

all: $(EXE)

clean:
	rm -f $(EXE)

$(EXE): $(SRC)
	$(CXX) $(CXXFLAGS) $(CXXFLAGSEXTRA) $(SRC) -o $@ 
