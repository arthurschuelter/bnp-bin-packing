CXX ?= c++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
# Override on the command line or export GUROBI_HOME before calling make.
GUROBI_HOME ?= /Library/gurobi1302/macos_universal2
GUROBI_LIB ?= gurobi130
CPPFLAGS += -I$(GUROBI_HOME)/include
LDFLAGS += -L$(GUROBI_HOME)/lib -Wl,-rpath,$(GUROBI_HOME)/lib
LDLIBS += -lgurobi_c++ -l$(GUROBI_LIB) -lm
SOURCES = main.cpp binpacksolver.cpp branchprice.cpp
HEADERS = binpacking.h binpacksolver.h branchprice.h
INPUT ?= ../testes/instancias/exemplo.txt
OUTPUT ?= ../saida.txt
TIME ?= 60
SEED ?= 1
.PHONY: all clean test run
all: binpack
binpack: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@
clean:
	rm -f binpack
test: all
	python3 ../testes/verificar.py --bin ./binpack
run: binpack
	./binpack -f "$(INPUT)" -o "$(OUTPUT)" -t "$(TIME)" -r "$(SEED)"
