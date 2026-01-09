CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -Wpedantic -g

CXXFLAGSEXTRA=

SRC=main.cpp
EXE=getline

all: $(EXE)

clean:
	rm -f $(EXE)

$(EXE): $(SRC)
	$(CXX) $(CXXFLAGS) $(CXXFLAGSEXTRA) $(SRC) -o $@ 
