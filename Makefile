CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

.PHONY: all demo seeded-demo test verify-demo clean
all: build/fundamentals
build:
	mkdir -p build
build/fundamentals: fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/matching_test: tests/matching_test.cpp fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) tests/matching_test.cpp -o $@
demo: build/fundamentals
	./build/fundamentals
seeded-demo: build/fundamentals
	./build/fundamentals --seed 8257
build/e2e_matching_test: tests/e2e_matching_test.cpp fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/order_identity_test: tests/order_identity_test.cpp fundamentals.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/fundamentals build/matching_test build/e2e_matching_test build/order_identity_test
	./build/matching_test
	./build/e2e_matching_test
	./build/order_identity_test
	bash tests/cli_test.sh ./build/fundamentals
verify-demo:
	python3 demo/verify_demo.py
clean:
	rm -rf build
