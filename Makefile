CXX ?= c++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all test clean
all: build/fundamentals

build:
	mkdir -p $@

build/fundamentals: fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/test_matching: test_matching.cpp fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

test: build/test_matching build/fundamentals
	./build/test_matching
	./build/fundamentals > /dev/null

clean:
	rm -rf build
