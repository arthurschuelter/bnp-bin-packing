CXX ?= c++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
# Detects Linux or macOS and the newest Gurobi installed in the default place.
# Override on the command line or export GUROBI_HOME/GUROBI_LIB before make.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
GUROBI_HOME ?= $(lastword $(sort $(wildcard /Library/gurobi*/macos_universal2)))
GUROBI_LIBEXT = dylib
else
GUROBI_HOME ?= $(lastword $(sort $(wildcard /opt/gurobi*/linux64)))
GUROBI_LIBEXT = so
endif
ifeq ($(strip $(GUROBI_HOME)),)
$(error Gurobi nao encontrado; use make GUROBI_HOME=/caminho/da/instalacao)
endif
# libgurobiXYZ.(so|dylib) -> gurobiXYZ, ignoring the _light variant.
GUROBI_LIB ?= $(patsubst lib%.$(GUROBI_LIBEXT),%,$(notdir $(lastword $(sort \
    $(filter-out %_light.$(GUROBI_LIBEXT),$(wildcard $(GUROBI_HOME)/lib/libgurobi[0-9]*.$(GUROBI_LIBEXT)))))))
CPPFLAGS += -I$(GUROBI_HOME)/include
LDFLAGS += -L$(GUROBI_HOME)/lib -Wl,-rpath,$(GUROBI_HOME)/lib
LDLIBS += -lgurobi_c++ -l$(GUROBI_LIB) -lm
SOURCES = main.cpp commandline.cpp binpacksolver.cpp branchprice.cpp
HEADERS = binpacking.h commandline.h binpacksolver.h branchprice.h
CLANG_TIDY ?= clang-tidy
CLANG_FORMAT ?= clang-format
TIDYFLAGS ?=
# Treat vendor headers as system headers for linting, keeping their warnings out.
LINTFLAGS = $(filter-out -I$(GUROBI_HOME)/include,$(CPPFLAGS)) $(CXXFLAGS) -isystem "$(GUROBI_HOME)/include"

TEST_FILE = medium-1
INPUT ?= ../testes/instancias/$(TEST_FILE).txt
OUTPUT ?= ../tests-results/$(TEST_FILE).txt

TIME ?= 60
SEED ?= 1

.PHONY: all clean test run lint format format-check
all: binpack
binpack: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@
clean:
	rm -f binpack
test: all
	python3 ../testes/verificar.py --bin ./binpack
run: binpack
	mkdir -p "$(dir $(OUTPUT))"
	./binpack -f "$(INPUT)" -o "$(OUTPUT)" -t "$(TIME)" -r "$(SEED)"
lint:
	$(CLANG_TIDY) $(TIDYFLAGS) $(SOURCES) -- $(LINTFLAGS)
format:
	$(CLANG_FORMAT) -i $(SOURCES) $(HEADERS)
format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(SOURCES) $(HEADERS)
