CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

.PHONY: all demo test clean
all: build/fundamentals
build:
	mkdir -p build
build/fundamentals: fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/matching_test: tests/matching_test.cpp fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) tests/matching_test.cpp -o $@
demo: build/fundamentals
	./build/fundamentals
test: build/matching_test
	./build/matching_test
clean:
	rm -rf build
