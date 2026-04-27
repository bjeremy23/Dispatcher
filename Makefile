CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -I src
LDFLAGS  := -lboost_system -lpthread

SRC_DIRS  := src/dispatcher src/socket src/msgbuffer src/signaler src/timer src/component
TEST_DIR  := test
PROC_DIR  := process

SRCS      := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.cpp))
OBJS      := $(SRCS:.cpp=.o)

TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(TEST_SRCS:.cpp=.o)
TEST_BIN  := run_tests

PROC_SRCS := $(wildcard $(PROC_DIR)/*.cpp)
PROC_OBJS := $(PROC_SRCS:.cpp=.o)
PROC_BIN  := pingpong

.PHONY: all test clean

all: $(TEST_BIN) $(PROC_BIN)

$(TEST_BIN): $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) -lgtest -lgtest_main

$(PROC_BIN): $(OBJS) $(PROC_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(PROC_OBJS) $(TEST_BIN) $(PROC_BIN)
