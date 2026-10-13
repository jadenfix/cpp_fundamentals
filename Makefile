CXX ?= c++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -Wpedantic
.PHONY: all test demo clean
all: build/fundamentals
build/fundamentals: fundamentals.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@
build/test_matching: test_matching.cpp fundamentals.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/test_matching
	./build/test_matching
demo: build/fundamentals
	./build/fundamentals
clean:
	rm -rf build
