# Compile with: make        Clean with: make clean
CXX      = g++
CXXFLAGS = -O2 -std=c++17 -Wall -Wextra

all: binpack

binpack: main.cpp binpacksolver.cpp binpacking.h binpacksolver.h
	$(CXX) $(CXXFLAGS) -o binpack main.cpp binpacksolver.cpp

clean:
	rm -f binpack
