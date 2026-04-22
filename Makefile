CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -I src
LDFLAGS  := -lboost_system -lpthread

SRC_DIRS := src/dispatcher src/socket src/msgbuffer
TEST_DIR := test

SRCS     := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.cpp))
OBJS     := $(SRCS:.cpp=.o)

TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(TEST_SRCS:.cpp=.o)
TEST_BIN  := run_tests

.PHONY: all test clean

all: $(TEST_BIN)

$(TEST_BIN): $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) -lgtest -lgtest_main

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TEST_BIN)
